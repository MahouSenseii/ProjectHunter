"""Give the LootChest source the rarity the project's own code says a chest has.

Every drop was Grade F, and FItemInitializationHelper deliberately skips affix
generation at Grade F - normal items have no affixes. That is why no item showed
affixes: not a broken generator, a loot source configured one grade too low.

The row said SourceRarity = DR_Common with no upgrade chance. The project's own
ULootSourceFunctionLibrary::GetDefaultSettingsForSourceType(LST_Chest) says a
chest is DR_Uncommon with RarityBonusChance 0.1 - that helper is dead code, never
called, so its intent never reached the data. These are its values, not invented.

Round-trips the table through JSON, which is the supported way to write rows.
Writes LootRarityStatus.json; Unreal exits 0 even when Python raises.
"""
import json
import traceback
from pathlib import Path

import unreal

TABLE = "/Game/ProjectHunter/World/LootTables/DT_LootSourceEntry"
ROW = "LootChest"

OUT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) \
    / "Automation" / "PH-20260831-39-AffixesAndChests"
OUT.mkdir(parents=True, exist_ok=True)
status = {"ok": False}

try:
    table = unreal.EditorAssetLibrary.load_asset(TABLE)
    if table is None:
        raise RuntimeError("Missing " + TABLE)

    exported = unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table)
    status["before"] = exported[:600]
    rows = json.loads(exported)

    changed = []
    for row in rows:
        if row.get("Name") != ROW:
            continue
        if row.get("SourceRarity") != "DR_Uncommon":
            changed.append({"field": "SourceRarity",
                            "from": row.get("SourceRarity"), "to": "DR_Uncommon"})
            row["SourceRarity"] = "DR_Uncommon"
        settings = row.get("DefaultSettings")
        if isinstance(settings, dict):
            old = settings.get("RarityBonusChance", 0.0)
            if float(old or 0.0) < 0.1:
                changed.append({"field": "DefaultSettings.RarityBonusChance",
                                "from": old, "to": 0.1})
                settings["RarityBonusChance"] = 0.1
        else:
            status["note"] = "DefaultSettings not a dict; only SourceRarity changed"

    status["changed"] = changed
    if changed:
        table.modify()
        if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(
                table, json.dumps(rows)):
            raise RuntimeError("fill_data_table_from_json_string refused the edited rows")
        if not unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False):
            raise RuntimeError("Save failed for " + TABLE)
        status["after"] = unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table)[:600]

    status["ok"] = True
    unreal.log("FixChestLootRarity: done")
except Exception as error:
    status["error"] = "".join(traceback.format_exception(type(error), error, error.__traceback__))
    unreal.log_error("FixChestLootRarity FAILED:\n" + status["error"])

(OUT / "LootRarityStatus.json").write_text(json.dumps(status, indent=2), encoding="utf-8")
