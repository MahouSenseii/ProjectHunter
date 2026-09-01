"""Render the authored System menu offscreen: one PNG per tab, and one per
Settings sub-tab.

Needs a real RHI: run with -RenderOffscreen, not -NullRHI.
Writes RenderStatus.json; Unreal exits 0 even when Python raises.
"""

import json
from pathlib import Path

import unreal

OUT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) \
    / "Automation" / "PH-20260831-35-SettingsTabs"
OUT.mkdir(parents=True, exist_ok=True)

SETTINGS_PAGE = "/Game/ProjectHunter/UI/Widgets/Menus/Settings/WBP_SettingsPage"
SECTIONS = (
    ("Gameplay", unreal.PHSettingsSection.SS_GAMEPLAY),
    ("Audio", unreal.PHSettingsSection.SS_AUDIO),
    ("KeyBinds", unreal.PHSettingsSection.SS_KEY_BINDS),
    ("Display", unreal.PHSettingsSection.SS_DISPLAY),
)

library = getattr(unreal, "PHMenuPreviewEditorLibrary", None)
status = {"ok": False}

if library is None:
    status["error"] = "PHMenuPreviewEditorLibrary missing; build ALS_ProjectHunterEditor"
else:
    status["validate"] = bool(library.validate_system_menu())
    status["Equipment"] = bool(library.render_system_menu(
        str(OUT / "Menu-Equipment.png"), 1920, 1080, unreal.MenuType.MT_EQUIPMENT))
    status["Stats"] = bool(library.render_system_menu(
        str(OUT / "Menu-Stats.png"), 1920, 1080, unreal.MenuType.MT_STATS))

    for label, section in SECTIONS:
        status["Settings-" + label] = bool(library.render_system_menu(
            str(OUT / ("Menu-Settings-%s.png" % label)), 1920, 1080,
            unreal.MenuType.MT_SETTINGS, section))

    status["ok"] = all(v is True for k, v in status.items() if k not in ("ok", "validate"))

(OUT / "RenderStatus.json").write_text(json.dumps(status, indent=2), encoding="utf-8")
unreal.log("RenderMenuPreview: " + json.dumps(status))
