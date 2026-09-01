"""Author the editable Hunter Paths graph and register its native menu page.

Run with -ExecutePythonScript while the normal editor is closed. The script never
replaces an existing asset; it updates the two task-owned assets and adds the page
through UPHMenuRootWidget's existing registration API.
"""

import json
import traceback
from pathlib import Path

import unreal

TREE_FOLDER = "/Game/ProjectHunter/Progression/PassiveTree"
TREE_PATH = TREE_FOLDER + "/DA_HunterPaths"
PAGE_FOLDER = "/Game/ProjectHunter/UI/Widgets/Menus/Passives"
PAGE_PATH = PAGE_FOLDER + "/WBP_PassiveTreePage"
ROOT_PATH = "/Game/ProjectHunter/UI/Widgets/Menus/System/WBP_SystemMenuRoot"

OUT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) \
    / "Automation" / "PH-20260831-45-PassiveNodeCards"
OUT.mkdir(parents=True, exist_ok=True)
status = {"ok": False, "assets": [], "nodes": [], "root_entries": []}


def set_prop(value, names, authored):
    last_error = None
    for name in names:
        try:
            value.set_editor_property(name, authored)
            return name
        except Exception as error:
            last_error = error
    raise RuntimeError("Could not set %s: %s" % (names, last_error))


def gameplay_tag(name):
    # TagName is immutable through set_editor_property and is not accepted as a
    # constructor keyword by UE 5.7's Python wrapper. import_text is the public
    # struct import path and also resolves native gameplay tags.
    value = unreal.GameplayTag()
    if not value.import_text(name) or not unreal.GameplayTagLibrary.is_gameplay_tag_valid(value):
        raise RuntimeError("Unknown gameplay tag: " + name)
    return value


def modifier(tag_name, magnitude):
    value = unreal.PHPassiveModifier()
    set_prop(value, ("attribute_tag",), gameplay_tag(tag_name))
    set_prop(value, ("magnitude",), float(magnitude))
    return value


def node(node_id, label, description, position, size, cost, required, modifiers, keywords):
    value = unreal.PHPassiveNodeDefinition()
    set_prop(value, ("node_id",), unreal.Name(node_id))
    set_prop(value, ("display_name",), label)
    set_prop(value, ("description",), description)
    set_prop(value, ("node_size",), size)
    set_prop(value, ("position",), unreal.Vector2D(float(position[0]), float(position[1])))
    set_prop(value, ("point_cost",), int(cost))
    set_prop(value, ("required_node_ids", "required_node_i_ds"), [unreal.Name(item) for item in required])
    set_prop(value, ("modifiers",), modifiers)
    set_prop(value, ("search_keywords",), keywords)
    return value


def ensure_data_asset():
    existing = unreal.EditorAssetLibrary.load_asset(TREE_PATH)
    if existing is not None:
        status["assets"].append({"asset": TREE_PATH, "action": "updated"})
        return existing
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.PHPassiveTreeDataAsset)
    created = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "DA_HunterPaths", TREE_FOLDER, unreal.PHPassiveTreeDataAsset, factory)
    if created is None:
        raise RuntimeError("Could not create " + TREE_PATH)
    status["assets"].append({"asset": TREE_PATH, "action": "created"})
    return created


def ensure_page():
    existing = unreal.EditorAssetLibrary.load_asset(PAGE_PATH)
    if existing is not None:
        status["assets"].append({"asset": PAGE_PATH, "action": "already existed"})
        return existing, False
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.PHPassiveTreeMenuPageWidget)
    created = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "WBP_PassiveTreePage", PAGE_FOLDER, unreal.WidgetBlueprint, factory)
    if created is None:
        raise RuntimeError("Could not create " + PAGE_PATH)
    status["assets"].append({"asset": PAGE_PATH, "action": "created"})
    return created, True


try:
    small = unreal.PHPassiveNodeSize.SMALL
    major = unreal.PHPassiveNodeSize.MAJOR
    nodes = [
        node("HunterAwakening", "Hunter Awakening",
             "The System recognizes your first chosen path. +1 Strength, Dexterity, Intelligence, and Endurance.",
             (0, 160), major, 1, [], [
                 modifier("Attributes.Primary.Strength", 1),
                 modifier("Attributes.Primary.Dexterity", 1),
                 modifier("Attributes.Primary.Intelligence", 1),
                 modifier("Attributes.Primary.Endurance", 1),
             ], ["start", "system", "all attributes"]),

        node("TemperedFrame", "Tempered Frame", "+2 Strength.",
             (-250, 60), small, 1, ["HunterAwakening"],
             [modifier("Attributes.Primary.Strength", 2)], ["vanguard", "power", "melee"]),
        node("SteadyGuard", "Steady Guard", "+2 Endurance.",
             (-500, -40), small, 1, ["TemperedFrame"],
             [modifier("Attributes.Primary.Endurance", 2)], ["vanguard", "stamina", "survival"]),
        node("IronPulse", "Iron Pulse", "+15 Maximum Health.",
             (-500, 160), small, 1, ["TemperedFrame"],
             [modifier("Attributes.Secondary.Vital.MaxHealth", 15)],
             ["vanguard", "health", "vital", "survival"]),
        node("CleavingForce", "Cleaving Force", "+5 Melee Damage.",
             (-760, -180), small, 1, ["SteadyGuard"],
             [modifier("Attributes.Secondary.Offensive.MeleeDamage", 5)],
             ["vanguard", "melee", "damage", "weapon"]),
        node("Gatebreaker", "Gatebreaker", "+5 Strength and +5 Endurance.",
             (-760, 40), major, 2, ["SteadyGuard"], [
                 modifier("Attributes.Primary.Strength", 5),
                 modifier("Attributes.Primary.Endurance", 5),
             ], ["vanguard", "major", "frontline"]),
        node("UnbrokenStance", "Unbroken Stance", "+10 Poise.",
             (-1020, 100), small, 1, ["Gatebreaker"],
             [modifier("Attributes.Secondary.Misc.Poise", 10)],
             ["vanguard", "poise", "stagger", "frontline"]),
        node("HoldTheLine", "Hold the Line", "+5 Global Defenses.",
             (-1020, -80), small, 1, ["Gatebreaker"],
             [modifier("Attributes.Secondary.Resistance.GlobalDefenses", 5)],
             ["vanguard", "defense", "armour", "resistance"]),
        node("LastBastion", "Last Bastion", "+10 Global Defenses and +20 Maximum Health.",
             (-1300, 20), major, 3, ["HoldTheLine"], [
                 modifier("Attributes.Secondary.Resistance.GlobalDefenses", 10),
                 modifier("Attributes.Secondary.Vital.MaxHealth", 20),
             ], ["vanguard", "major", "defense", "health", "survival"]),

        node("KeenEye", "Keen Eye", "+2 Dexterity.",
             (0, -80), small, 1, ["HunterAwakening"],
             [modifier("Attributes.Primary.Dexterity", 2)], ["tracker", "speed", "critical"]),
        node("SwiftStep", "Swift Step", "+10 Movement Speed.",
             (250, -210), small, 1, ["KeenEye"],
             [modifier("Attributes.Secondary.Misc.MovementSpeed", 10)],
             ["tracker", "movement", "speed", "dodge"]),
        node("RelentlessPursuit", "Relentless Pursuit", "+20 Movement Speed.",
             (0, -340), small, 1, ["KeenEye"],
             [modifier("Attributes.Secondary.Misc.MovementSpeed", 20)], ["tracker", "movement", "chase"]),
        node("PredatorFocus", "Predator Focus", "+10 Critical Multiplier.",
             (-250, -480), small, 1, ["RelentlessPursuit"],
             [modifier("Attributes.Secondary.Offensive.CritMultiplier", 10)],
             ["tracker", "critical", "focus", "hunt"]),
        node("ApexTracker", "Apex Tracker", "+5 Dexterity and +5 Critical Chance.",
             (0, -620), major, 2, ["RelentlessPursuit"], [
                 modifier("Attributes.Primary.Dexterity", 5),
                 modifier("Attributes.Secondary.Offensive.CritChance", 5),
             ], ["tracker", "major", "critical", "boss"]),
        node("AmbushInstinct", "Ambush Instinct", "+5 Attack Speed.",
             (270, -760), small, 1, ["ApexTracker"],
             [modifier("Attributes.Secondary.Offensive.AttackSpeed", 5)],
             ["tracker", "attack speed", "ambush", "hunter"]),
        node("ExecutionWindow", "Execution Window", "+10 Damage while at low Health.",
             (0, -900), small, 1, ["ApexTracker"],
             [modifier("Attributes.Secondary.Offensive.DamageBonusWhileAtLowHP", 10)],
             ["tracker", "execute", "low health", "damage"]),
        node("FinalMark", "Final Mark", "+8 Ranged Damage and +5 Critical Chance.",
             (0, -1190), major, 3, ["ExecutionWindow"], [
                 modifier("Attributes.Secondary.Offensive.RangedDamage", 8),
                 modifier("Attributes.Secondary.Offensive.CritChance", 5),
             ], ["tracker", "major", "mark", "ranged", "critical"]),

        node("GateSense", "Gate Sense", "+2 Intelligence.",
             (250, 60), small, 1, ["HunterAwakening"],
             [modifier("Attributes.Primary.Intelligence", 2)], ["scholar", "gate", "mana"]),
        node("ManaThread", "Mana Thread", "+3 Mana Regeneration.",
             (500, 160), small, 1, ["GateSense"],
             [modifier("Attributes.Secondary.Vital.ManaRegenAmount", 3)],
             ["scholar", "mana", "regeneration", "gate"]),
        node("ArcaneReserve", "Arcane Reserve", "+15 Maximum Mana.",
             (500, -40), small, 1, ["GateSense"],
             [modifier("Attributes.Secondary.Vital.MaxMana", 15)], ["scholar", "mana", "skill"]),
        node("RiftStudy", "Rift Study", "+6 Spell Damage.",
             (760, -180), small, 1, ["ArcaneReserve"],
             [modifier("Attributes.Secondary.Offensive.SpellDamage", 6)],
             ["scholar", "spell", "damage", "rift"]),
        node("WorldReader", "World Reader", "+5 Intelligence and +2 Luck.",
             (760, 40), major, 2, ["ArcaneReserve"], [
                 modifier("Attributes.Primary.Intelligence", 5),
                 modifier("Attributes.Primary.Luck", 2),
             ], ["scholar", "major", "gate", "loot", "knowledge"]),
        node("VeilSight", "Veil Sight", "+15 Maximum Arcane Shield.",
             (1020, 100), small, 1, ["WorldReader"],
             [modifier("Attributes.Secondary.Vital.MaxArcaneShield", 15)],
             ["scholar", "arcane shield", "veil", "survival"]),
        node("ConstellationWard", "Constellation Ward", "+5 Corruption Resistance.",
             (1020, -80), small, 1, ["WorldReader"],
             [modifier("Attributes.Secondary.Resistance.Corruption.Flat", 5)],
             ["scholar", "constellation", "corruption", "resistance"]),
        node("GateboundMind", "Gatebound Mind", "+10 Elemental Damage and +20 Maximum Mana.",
             (1300, 20), major, 3, ["ConstellationWard"], [
                 modifier("Attributes.Secondary.Offensive.ElementalDamage", 10),
                 modifier("Attributes.Secondary.Vital.MaxMana", 20),
             ], ["scholar", "major", "gate", "elemental", "mana"]),
    ]

    tree = ensure_data_asset()
    tree.modify()
    tree.set_editor_property("tree_id", unreal.Name("HunterPaths"))
    tree.set_editor_property("display_name", "Hunter Paths")
    tree.set_editor_property("nodes", nodes)
    status["nodes"] = [str(item.get_editor_property("node_id")) for item in nodes]

    page, page_created = ensure_page()
    if page_created:
        unreal.BlueprintEditorLibrary.compile_blueprint(page)

    root = unreal.EditorAssetLibrary.load_asset(ROOT_PATH)
    if root is None:
        raise RuntimeError("Missing menu root: " + ROOT_PATH)
    defaults = unreal.get_default_object(unreal.EditorAssetLibrary.load_blueprint_class(root.get_path_name()))
    page_class = unreal.EditorAssetLibrary.load_blueprint_class(page.get_path_name())
    entries = defaults.get_editor_property("menu_entries") or []
    root_changed = not any(
        entry.get_editor_property("menu_type") == unreal.MenuType.MT_PASSIVE_TREE
        and entry.get_editor_property("widget_class") == page_class
        for entry in entries)
    if root_changed:
        defaults.modify()
        defaults.set_menu_page_widget_class(unreal.MenuType.MT_PASSIVE_TREE, page_class)
        root.modify()
        unreal.BlueprintEditorLibrary.compile_blueprint(root)

    assets_to_save = [tree]
    if page_created:
        assets_to_save.append(page)
    if root_changed:
        assets_to_save.append(root)
    for asset in assets_to_save:
        if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
            raise RuntimeError("Save failed: " + asset.get_path_name())

    status["root_entries"] = [
        {"menu_type": str(entry.get_editor_property("menu_type")),
         "widget_class": str(entry.get_editor_property("widget_class"))}
        for entry in (defaults.get_editor_property("menu_entries") or [])]
    status["ok"] = True
    unreal.log("SetupPassiveTree: authored %d nodes" % len(nodes))
except Exception as error:
    status["error"] = "".join(traceback.format_exception(type(error), error, error.__traceback__))
    unreal.log_error("SetupPassiveTree FAILED:\n" + status["error"])

(OUT / "SetupStatus.json").write_text(json.dumps(status, indent=2), encoding="utf-8")
