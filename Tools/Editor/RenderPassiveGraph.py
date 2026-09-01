"""Render the passive tree as it looks in the graph editor."""

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
    status["render_graph"] = bool(library.render_passive_tree_graph(
        str(out / "PassiveGraph-Editor-1920.png"), 1920, 1080))
    status["ok"] = status["render_graph"]

(out / "GraphRenderStatus.json").write_text(json.dumps(status, indent=2), encoding="utf-8")
unreal.log("RenderPassiveGraph: " + json.dumps(status))
