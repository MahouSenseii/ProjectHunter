"""Give the player-facing Input Actions a mappable name.

Enhanced Input's rebinding needs a MappingName, which comes from an Input
Action's PlayerMappableKeySettings. Without it the key profile is empty and the
Key Binds tab has nothing to show.

These Input Actions are ALS plugin content. The change is additive - it sets one
previously-empty property and touches no key, trigger, modifier or value type -
and every asset is backed up first. Movement and camera axes are deliberately
left alone: they are axis mappings, and rebinding them needs a different control
than "press a key".

Writes MarkStatus.json; Unreal exits 0 even when Python raises.
"""

import json
import shutil
import traceback
from pathlib import Path

import unreal

ACTION_DIR = "/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/Input/Default"
PLUGIN_CONTENT = Path(unreal.Paths.convert_relative_path_to_full(
    unreal.Paths.project_plugins_dir())) / "ALS-Community" / "Content" / \
    "AdvancedLocomotionV4" / "Blueprints" / "Input" / "Default"

OUT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) \
    / "Automation" / "PH-20260831-35-SettingsTabs"
BACKUP = OUT / "InputActionBackup"

# Discrete actions a player would expect to rebind, with the label the menu shows.
ACTIONS = {
    "JumpAction": "Jump",
    "SprintAction": "Sprint",
    "WalkAction": "Walk",
    "AimAction": "Aim",
    "AttackAction": "Attack",
    "DashAction": "Dash",
    "Interact": "Interact",
    "Menu": "Open Menu",
    "StanceAction": "Crouch / Stance",
    "RagdollAction": "Ragdoll",
}

OUT.mkdir(parents=True, exist_ok=True)
BACKUP.mkdir(parents=True, exist_ok=True)
status = {"ok": False, "marked": [], "skipped": []}

try:
    saveables = []
    for asset_name, label in ACTIONS.items():
        path = ACTION_DIR + "/" + asset_name
        action = unreal.EditorAssetLibrary.load_asset(path)
        if action is None:
            status["skipped"].append({"asset": path, "reason": "not found"})
            continue

        source = PLUGIN_CONTENT / (asset_name + ".uasset")
        if source.exists():
            shutil.copy2(source, BACKUP / source.name)

        existing = action.get_editor_property("player_mappable_key_settings")
        if existing is not None:
            status["skipped"].append({"asset": path, "reason": "already mappable"})
            continue

        settings = unreal.PlayerMappableKeySettings(action)
        settings.set_editor_property("name", unreal.Name(asset_name))
        settings.set_editor_property("display_name", unreal.Text(label))
        settings.set_editor_property("display_category", unreal.Text("Gameplay"))

        action.modify()
        action.set_editor_property("player_mappable_key_settings", settings)
        saveables.append((path, action))
        status["marked"].append({"asset": path, "mapping_name": asset_name, "label": label})

    for path, action in saveables:
        if not unreal.EditorAssetLibrary.save_loaded_asset(action, only_if_is_dirty=False):
            raise RuntimeError("Save failed: " + path)

    status["ok"] = True
    unreal.log("MarkMappableActions: marked %d" % len(status["marked"]))
except Exception as error:
    status["error"] = "".join(traceback.format_exception(type(error), error, error.__traceback__))
    unreal.log_error("MarkMappableActions FAILED:\n" + status["error"])

(OUT / "MarkStatus.json").write_text(json.dumps(status, indent=2), encoding="utf-8")
