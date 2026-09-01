"""Create the Stats and Settings menu pages and register them with the menu root.

Run with -ExecutePythonScript on a closed-editor project. Writes
SetupStatus.json with an explicit ok flag: Unreal exits 0 even when Python
raises, so the exit code proves nothing.

The pages build their own rows in C++, so these Widget Blueprints are
deliberately empty shells. They exist so the look can be extended in UMG later
and so the menu references an asset rather than a raw native class.

UPHMenuRootWidget already builds its tabs from EMenuType, so the tabs are
present already; what this adds is the page class each one opens.
"""

import json
import traceback
from pathlib import Path

import unreal

MENU_ROOT = "/Game/ProjectHunter/UI/Widgets/Menus/System/WBP_SystemMenuRoot"
PAGES = (
    ("WBP_StatsPage", "/Game/ProjectHunter/UI/Widgets/Menus/Stats",
     unreal.PHStatsMenuPageWidget, unreal.MenuType.MT_STATS, "Stats"),
    ("WBP_SettingsPage", "/Game/ProjectHunter/UI/Widgets/Menus/Settings",
     unreal.PHSettingsMenuPageWidget, unreal.MenuType.MT_SETTINGS, "Settings"),
)

OUT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) \
    / "Automation" / "PH-20260831-34-StatsSettingsPages"
OUT.mkdir(parents=True, exist_ok=True)
status = {"ok": False, "created": [], "entries": []}


def ensure_page(name, folder, parent_class):
    """Create the page Blueprint if absent; never replace an existing one."""
    path = folder + "/" + name
    existing = unreal.EditorAssetLibrary.load_asset(path)
    if existing is not None:
        status["created"].append({"asset": path, "action": "already existed"})
        return existing

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, folder, unreal.WidgetBlueprint, factory)
    if asset is None:
        raise RuntimeError("Could not create " + path)
    status["created"].append({"asset": path, "action": "created"})
    return asset


try:
    root = unreal.EditorAssetLibrary.load_asset(MENU_ROOT)
    if root is None:
        raise RuntimeError("Missing menu root: " + MENU_ROOT)

    saveables = []
    defaults = unreal.get_default_object(
        unreal.EditorAssetLibrary.load_blueprint_class(root.get_path_name()))
    defaults.modify()

    for name, folder, parent_class, menu_type, label in PAGES:
        page = ensure_page(name, folder, parent_class)
        unreal.BlueprintEditorLibrary.compile_blueprint(page)
        saveables.append(page)

        # The root's own API. FMenuEntry.MenuType is EditDefaultsOnly, so a
        # Python-built struct cannot set it; this also preserves the existing
        # Equipment entry and any authored labels or icons rather than
        # rebuilding the array and silently dropping them.
        defaults.set_menu_page_widget_class(
            menu_type,
            unreal.EditorAssetLibrary.load_blueprint_class(page.get_path_name()))

    root.modify()
    unreal.BlueprintEditorLibrary.compile_blueprint(root)
    saveables.append(root)

    status["entries"] = [
        {"menu_type": str(e.get_editor_property("menu_type")),
         "widget_class": str(e.get_editor_property("widget_class"))}
        for e in (defaults.get_editor_property("menu_entries") or [])]

    for asset in saveables:
        if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
            raise RuntimeError("Save failed: " + asset.get_path_name())

    status["ok"] = True
    unreal.log("SetupMenuPages: done")
except Exception as error:
    status["error"] = "".join(traceback.format_exception(type(error), error, error.__traceback__))
    unreal.log_error("SetupMenuPages FAILED:\n" + status["error"])

(OUT / "SetupStatus.json").write_text(json.dumps(status, indent=2), encoding="utf-8")
