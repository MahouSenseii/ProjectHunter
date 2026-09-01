"""Unattended entry point for PolishSystemMenu, for a headless editor run.

Run with -ExecutePythonScript (an initialized editor with Slate), never with
-run=pythonscript. Close every other editor of this project first: this saves
assets, and a second editor holding them will fight it.

Unreal can exit zero even when a Python exception was raised, so this never
relies on the process exit code. It writes ApplyStatus.json beside the run's
evidence directory with an explicit ok flag and the full traceback on failure -
check that file, not the exit code.
"""

import json
import sys
import traceback
from datetime import datetime
from pathlib import Path

import unreal

EVIDENCE = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) \
    / "Automation" / "PH-20260831-33-MenuSystemLook"

sys.path.insert(0, str(Path(unreal.Paths.convert_relative_path_to_full(
    unreal.Paths.project_dir())) / "Tools" / "Editor"))

import PolishSystemMenu  # noqa: E402  (needs the path above)

EVIDENCE.mkdir(parents=True, exist_ok=True)
run_dir = EVIDENCE / ("Apply-" + datetime.now().strftime("%Y%m%d-%H%M%S"))
status = {"output": str(run_dir), "ok": False}

try:
    PolishSystemMenu.main(["--apply", "--output", str(run_dir)])
    status["ok"] = True
    unreal.log("ApplyMenuPolish: succeeded -> " + str(run_dir))
except Exception as error:  # recorded, not swallowed - see the traceback below
    status["error"] = "".join(traceback.format_exception(type(error), error, error.__traceback__))
    unreal.log_error("ApplyMenuPolish FAILED:\n" + status["error"])

(EVIDENCE / "ApplyStatus.json").write_text(json.dumps(status, indent=2), encoding="utf-8")
