#include "Generation/PHGenerationTags.h"

namespace PHGenerationTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor, "Anchor", "Root for semantic generation anchors.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_PlayerStart, "Anchor.PlayerStart", "Candidate player entry pose.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Exit, "Anchor.Exit", "Candidate floor exit pose.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Enemy, "Anchor.Enemy", "Enemy spawn pose of unspecified size.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Enemy_Small, "Anchor.Enemy.Small", "Enemy pose with a small footprint.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Enemy_Medium, "Anchor.Enemy.Medium", "Enemy pose with a medium footprint.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Enemy_Large, "Anchor.Enemy.Large", "Enemy pose with a large footprint.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Elite, "Anchor.Elite", "Elite spawn pose.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Boss, "Anchor.Boss", "Boss spawn pose.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Chest, "Anchor.Chest", "Container pose.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Treasure, "Anchor.Treasure", "Reward pose.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Event, "Anchor.Event", "Dynamic-event pose of unspecified scale.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Event_Small, "Anchor.Event.Small", "Event needing little space.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Event_Medium, "Anchor.Event.Medium", "Event needing moderate space.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Event_Large, "Anchor.Event.Large", "Event needing open space.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_NPC, "Anchor.NPC", "Non-combat NPC pose.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Rescue, "Anchor.Rescue", "Pose for a rescuable character.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Ambush, "Anchor.Ambush", "Pose an ambush triggers from.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Defend, "Anchor.Defend", "Pose a defend objective protects.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Escort_Start, "Anchor.Escort.Start", "Where an escort begins.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Escort_End, "Anchor.Escort.End", "Where an escort ends.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Secret, "Anchor.Secret", "Pose for concealed content.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Trap, "Anchor.Trap", "Trap pose.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Portal, "Anchor.Portal", "Portal pose.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Shrine, "Anchor.Shrine", "Shrine pose.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anchor_Constellation, "Anchor.Constellation", "Pose a Constellation may manifest or intervene at.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Piece, "Piece", "Root for logical construction pieces.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Piece_Wall, "Piece.Wall", "Any wall piece.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Piece_Wall_Straight, "Piece.Wall.Straight", "Straight wall span.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Piece_Wall_Corner, "Piece.Wall.Corner", "Wall corner.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Piece_Floor, "Piece.Floor", "Floor tile.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Piece_Ceiling, "Piece.Ceiling", "Ceiling tile.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Piece_Door, "Piece.Door", "Doorway between regions.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Piece_Stair, "Piece.Stair", "Vertical connector.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Piece_Pillar, "Piece.Pillar", "Free-standing support.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Piece_Room_Small, "Piece.Room.Small", "Prefabricated small room shell.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Piece_Room_Large, "Piece.Room.Large", "Prefabricated large room shell.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Piece_Trap, "Piece.Trap", "Trap module.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prop, "Prop", "Root for decoration props.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prop_Barrel, "Prop.Barrel", "Barrel.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prop_Barrel_Explosive, "Prop.Barrel.Explosive", "Explosive barrel.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prop_Crate, "Prop.Crate", "Crate or box.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prop_Container, "Prop.Container", "Large container.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prop_Debris, "Prop.Debris", "Scattered debris.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prop_Rubble, "Prop.Rubble", "Rubble pile.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prop_Clutter_Small, "Prop.Clutter.Small", "Small floor clutter.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prop_Furniture, "Prop.Furniture", "Furniture.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prop_Vegetation, "Prop.Vegetation", "Plant or tree.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prop_Pipe, "Prop.Pipe", "Pipework dressing.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prop_Light, "Prop.Light", "Light fixture that stands on the floor or hangs on a wall.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prop_Light_Ceiling, "Prop.Light.Ceiling", "Fixture hung at the planned light poses rather than scattered on the floor.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prop_Banner, "Prop.Banner", "Hanging banner or cloth.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prop_Corpse, "Prop.Corpse", "Corpse or remains dressing.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prop_Campfire, "Prop.Campfire", "Campfire or brazier.");
}
