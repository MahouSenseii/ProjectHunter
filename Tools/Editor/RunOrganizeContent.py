"""Headless entry point for OrganizeContent.

Reads PH_ORGANIZE_GROUP and PH_ORGANIZE_APPLY from the environment so one
script can drive every group without editing a constant between runs.
Unreal exits 0 even when Python raises: read the group's JSON, not the code.
"""
import os
import sys
from pathlib import Path

import unreal

sys.path.insert(0, str(Path(unreal.Paths.convert_relative_path_to_full(
    unreal.Paths.project_dir())) / "Tools" / "Editor"))

import OrganizeContent  # noqa: E402

group = os.environ.get("PH_ORGANIZE_GROUP", "core")
argv = ["--group", group]
if os.environ.get("PH_ORGANIZE_APPLY", "") == "1":
    argv.append("--apply")
OrganizeContent.main(argv)
