"""Set the generated floor's ChestClass so Anchor.Chest poses actually spawn a chest.

APHGeneratedFloorActor::PlaceChests returns early when ChestClass is unset - by
design, so a floor can have no containers - and the placed GeneratedFloor in
L_BlockoutTest had it at None. The Anchor.Chest rule was already authored, so the
layout was seating chest poses that nothing consumed.

Writes ChestFixStatus.json; Unreal exits 0 even when Python raises.
"""
import json
import traceback
from pathlib import Path

import unreal

LEVEL = "/Game/ProjectHunter/World/Tower/Maps/L_BlockoutTest"
CHEST_BP = "/Game/ProjectHunter/Interactables/Chest/BP_BasicChest"

OUT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) \
    / "Automation" / "PH-20260831-39-AffixesAndChests"
OUT.mkdir(parents=True, exist_ok=True)
status = {"ok": False, "changed": []}

try:
    chest_class = unreal.EditorAssetLibrary.load_blueprint_class(CHEST_BP)
    if chest_class is None:
        raise RuntimeError("Missing chest Blueprint: " + CHEST_BP)
    if not unreal.MathLibrary.class_is_child_of(chest_class, unreal.LootChest):
        raise RuntimeError("BP_BasicChest is not an ALootChest; PlaceChests would reject it")

    unreal.EditorLoadingAndSavingUtils.load_map(LEVEL)
    sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    for actor in sub.get_all_level_actors():
        if not isinstance(actor, unreal.PHGeneratedFloorActor):
            continue
        before = str(actor.get_editor_property("chest_class"))
        if "BP_BasicChest" in before:
            status["changed"].append({"actor": actor.get_actor_label(),
                                      "action": "already set"})
            continue
        actor.modify()
        actor.set_editor_property("chest_class", chest_class)
        status["changed"].append({"actor": actor.get_actor_label(),
                                  "from": before, "to": CHEST_BP})

    if status["changed"]:
        unreal.EditorLoadingAndSavingUtils.save_map(
            unreal.EditorLevelLibrary.get_editor_world(), "")
    status["ok"] = True
    unreal.log("FixChestClass: done")
except Exception as error:
    status["error"] = "".join(traceback.format_exception(type(error), error, error.__traceback__))
    unreal.log_error("FixChestClass FAILED:\n" + status["error"])

(OUT / "ChestFixStatus.json").write_text(json.dumps(status, indent=2), encoding="utf-8")
