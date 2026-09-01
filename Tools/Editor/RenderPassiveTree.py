"""Render the actual Passive Tree tab through the existing menu preview fixture."""

import json
from pathlib import Path

import unreal

out = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) \
    / "Automation" / "PH-20260831-45-PassiveNodeCards"
out.mkdir(parents=True, exist_ok=True)

library = getattr(unreal, "PHMenuPreviewEditorLibrary", None)
status = {"ok": False}
if library is None:
    status["error"] = "PHMenuPreviewEditorLibrary is unavailable; build the editor module first."
else:
    status["validate_system_menu"] = bool(library.validate_system_menu())
    status["render_passive_tree"] = bool(library.render_system_menu(
        str(out / "Menu-PassiveTree-Cards-1920.png"), 1920, 1080,
        unreal.MenuType.MT_PASSIVE_TREE))
    status["ok"] = status["validate_system_menu"] and status["render_passive_tree"]

(out / "RenderStatus.json").write_text(json.dumps(status, indent=2), encoding="utf-8")
unreal.log("RenderPassiveTree: " + json.dumps(status))
