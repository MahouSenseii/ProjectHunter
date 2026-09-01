"""Reorganize /Game/ProjectHunter, one group at a time.

Run with -ExecutePythonScript on a closed editor, via RunOrganizeContent.py.
Writes Organize-<group>.json with the full source -> destination mapping, which
is what makes the work reversible: feed the mapping back the other way.

Never move .uasset files on the filesystem. A hard reference stores a package
path, so a filesystem move silently breaks every referencer with no error until
something fails to load. rename_asset rewrites the referencers.

Two lessons are baked in after a first attempt crashed the editor:

- **Per-asset renames, not rename_directory.** The directory call put assets at
  a path that did not match their source layout (a Spells/ subtree arrived under
  UI/Spells/). Every destination here is computed from the source package path,
  so the result is predictable.
- **One group per process, and save only what moved.** The first attempt moved
  1,526 assets and then saved the whole 2,400-asset tree in one go, and died in
  CoreUObject. Groups are small enough to verify, and a crash costs one group.

/Game/ProjectHunter/Combat is off limits by the user's instruction, and that is
asserted rather than assumed.
"""

import argparse
import json
from datetime import datetime
from pathlib import Path

import unreal

ROOT = "/Game/ProjectHunter"
PROTECTED = ROOT + "/Combat"

# Each group is a list of (source directory, destination directory).
GROUPS = {
    # Small, structural, and the ones the user named. Safe to do first.
    "core": [
        (ROOT + "/Generation", ROOT + "/World/Tower/Generation"),
        (ROOT + "/Libary",     ROOT + "/Library"),
        (ROOT + "/Data/Stats", ROOT + "/Gameplay/Stats"),
        (ROOT + "/GAS/Init",   ROOT + "/Gameplay/Effects"),
    ],
    # Item art next to the items it belongs to.
    "itemart": [
        (ROOT + "/Textures/Items",     ROOT + "/Item/Art/Items"),
        (ROOT + "/Textures/Equipment", ROOT + "/Item/Art/Equipment"),
    ],
    # The two unreferenced copies of the same pack, out of the project's own
    # folder. One per run: together they are 1,526 assets.
    "packA": [(ROOT + "/Textures/Assets", "/Game/Imported/AssetPack_FromTextures")],
    "packB": [(ROOT + "/Textures/Materials/Assets",
               "/Game/Imported/AssetPack_FromTexturesMaterials")],
}

# Assets sitting loose directly in /Textures get filed under /Textures/UI.
LOOSE_SOURCE = ROOT + "/Textures"
LOOSE_DESTINATION = ROOT + "/Textures/UI"

OUT_ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) \
    / "Automation" / "PH-20260831-37-ContentOrganize"


def assert_not_protected(path):
    if path == PROTECTED or path.startswith(PROTECTED + "/"):
        raise RuntimeError("Refusing to touch protected path: " + path)


def registry():
    return unreal.AssetRegistryHelpers.get_asset_registry()


def tree_moves(source_root, destination_root):
    """Every asset under source_root, with its destination computed explicitly."""
    moves = []
    for data in registry().get_assets_by_path(unreal.Name(source_root), recursive=True):
        package = str(data.package_name)
        if not package.startswith(source_root + "/"):
            continue
        relative = package[len(source_root) + 1:]
        moves.append((package, destination_root + "/" + relative))
    return sorted(moves)


def loose_moves():
    """Assets directly in /Textures, excluding anything already in a subfolder."""
    moves = []
    for data in registry().get_assets_by_path(unreal.Name(LOOSE_SOURCE), recursive=False):
        package = str(data.package_name)
        moves.append((package, LOOSE_DESTINATION + "/" + package.rsplit("/", 1)[1]))
    return sorted(moves)


def count_redirectors(package_paths):
    """How many redirector stubs the renames left behind.

    UE 5.7's Python AssetTools exposes no fixup_referencers, so these are not
    collapsed here. They are harmless - old paths keep resolving - and
    FixupRedirects.ps1 runs the ResavePackages commandlet to clear them.
    """
    total = 0
    for path in package_paths:
        search = unreal.ARFilter(
            class_paths=[unreal.TopLevelAssetPath("/Script/CoreUObject", "ObjectRedirector")],
            package_paths=[path], recursive_paths=True)
        total += len(registry().get_assets(search))
    return total


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--group", required=True,
                        choices=sorted(GROUPS.keys()) + ["loose"])
    parser.add_argument("--apply", action="store_true")
    args = parser.parse_args(argv)

    OUT_ROOT.mkdir(parents=True, exist_ok=True)
    status = {
        "group": args.group,
        "applied": args.apply,
        "when": datetime.now().isoformat(timespec="seconds"),
        "moves": [], "skipped": [], "failed": [], "ok": False,
    }

    try:
        if args.group == "loose":
            planned = loose_moves()
            touched_roots = [LOOSE_SOURCE]
        else:
            planned = []
            touched_roots = []
            for source, destination in GROUPS[args.group]:
                assert_not_protected(source)
                assert_not_protected(destination)
                if not unreal.EditorAssetLibrary.does_directory_exist(source):
                    status["skipped"].append({"from": source, "reason": "source missing"})
                    continue
                planned.extend(tree_moves(source, destination))
                touched_roots.append(source)

        for source, destination in planned:
            assert_not_protected(source)
            assert_not_protected(destination)

        for source, destination in planned:
            if unreal.EditorAssetLibrary.does_asset_exist(destination):
                # Already moved by an earlier run; groups are resumable.
                status["skipped"].append({"from": source, "reason": "destination exists"})
                continue
            if not unreal.EditorAssetLibrary.does_asset_exist(source):
                status["skipped"].append({"from": source, "reason": "source missing"})
                continue

            if not args.apply:
                status["moves"].append({"from": source, "to": destination})
                continue

            # AssetTools batch rename rather than EditorAssetLibrary.rename_asset:
            # the latter silently returns false for a level (.umap) and moved
            # nothing, which is how L_BlockoutTest was left behind on the first run.
            # Some assets in the imported packs are damaged - broken USkeleton
            # references - and loading one raises rather than returning None.
            # An unhandled raise here aborted the whole group, and loading them
            # in bulk is the likeliest reason the first attempt crashed the
            # editor outright. Record and step over them.
            try:
                asset = unreal.EditorAssetLibrary.load_asset(source)
            except Exception as load_error:
                status["failed"].append({"from": source, "to": destination,
                                         "reason": "load raised: "
                                                   + str(load_error).splitlines()[0][:160]})
                continue
            if asset is None:
                status["failed"].append({"from": source, "to": destination,
                                         "reason": "could not load"})
                continue
            folder, name = destination.rsplit("/", 1)
            try:
                renamed = unreal.AssetToolsHelpers.get_asset_tools().rename_assets(
                    [unreal.AssetRenameData(asset, folder, name)])
            except Exception as rename_error:
                status["failed"].append({"from": source, "to": destination,
                                         "reason": "rename raised: "
                                                   + str(rename_error).splitlines()[0][:160]})
                continue
            if renamed and unreal.EditorAssetLibrary.does_asset_exist(destination):
                status["moves"].append({"from": source, "to": destination})
            else:
                status["failed"].append({"from": source, "to": destination,
                                         "reason": "rename_assets refused"})

        if args.apply and status["moves"]:
            # Save only what this group produced, not the whole project.
            destinations = sorted({m["to"].rsplit("/", 1)[0] for m in status["moves"]})
            for directory in destinations:
                unreal.EditorAssetLibrary.save_directory(directory, False, True)
            status["redirectors_left"] = count_redirectors(touched_roots)

        status["ok"] = not status["failed"]
    except Exception as error:
        import traceback
        status["error"] = "".join(
            traceback.format_exception(type(error), error, error.__traceback__))
        unreal.log_error("OrganizeContent FAILED:\n" + status["error"])

    suffix = "apply" if args.apply else "plan"
    (OUT_ROOT / ("Organize-%s-%s.json" % (args.group, suffix))).write_text(
        json.dumps(status, indent=2), encoding="utf-8")
    unreal.log("OrganizeContent %s %s: %d moves, %d failed"
               % (args.group, suffix, len(status["moves"]), len(status["failed"])))


if __name__ == "__main__":
    main()
