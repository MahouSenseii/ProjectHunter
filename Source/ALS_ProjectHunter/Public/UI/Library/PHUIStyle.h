// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"

/**
 * One source of truth for the system-window palette.
 *
 * Every value is LINEAR, converted from the sRGB hex noted beside it. Pasting
 * a hex value straight into an FLinearColor is the usual way these end up
 * looking washed out - #2E9BE0 is (0.027, 0.328, 0.745) linear, not
 * (0.18, 0.61, 0.88).
 *
 * The UMG side of the menu gets the same palette from the chamfered panel
 * textures, so anything set in C++ has to agree with those or the two halves
 * of the menu drift apart.
 */
namespace PHUIStyle
{
	// SURFACES

	/** #2E9BE0 - the panel body baked into T_SystemPanel_Fill. */
	inline constexpr FLinearColor Azure{0.0273f, 0.3278f, 0.7454f, 1.0f};

	/** #1C4F9E - recessed elements: slot interiors, unselected tabs. */
	inline constexpr FLinearColor AzureDeep{0.0116f, 0.0782f, 0.3419f, 1.0f};

	/** Outlines and body text. The window's line language is white, not cyan. */
	inline constexpr FLinearColor Line{1.0f, 1.0f, 1.0f, 0.80f};
	inline constexpr FLinearColor TextPrimary{1.0f, 1.0f, 1.0f, 1.0f};
	inline constexpr FLinearColor TextDim{0.78f, 0.92f, 1.0f, 1.0f};

	// ITEM GRADES
	//
	// F through SS is the manhwa grading ladder, so the ramp has to be readable
	// at a glance and each step obviously distinct from its neighbours. Grey ->
	// white -> green -> cyan -> violet -> amber -> orange -> crimson, with the
	// two off-ladder states pulled well away from all of them.

	inline constexpr FLinearColor GradeF{0.3515f, 0.4020f, 0.4564f, 1.0f};   // #A0AAB4
	inline constexpr FLinearColor GradeE{1.0f, 1.0f, 1.0f, 1.0f};            // #FFFFFF
	inline constexpr FLinearColor GradeD{0.0685f, 0.7305f, 0.2159f, 1.0f};   // #4ADE80
	inline constexpr FLinearColor GradeC{0.0395f, 0.5089f, 0.9387f, 1.0f};   // #38BDF8
	inline constexpr FLinearColor GradeB{0.3916f, 0.0908f, 0.9301f, 1.0f};   // #A855F7
	inline constexpr FLinearColor GradeA{0.9647f, 0.5210f, 0.0176f, 1.0f};   // #FBBF24
	inline constexpr FLinearColor GradeS{0.9647f, 0.1746f, 0.0168f, 1.0f};   // #FB7423
	inline constexpr FLinearColor GradeSS{0.8632f, 0.0437f, 0.0762f, 1.0f};  // #EF3B4E

	/** Unidentified: deliberately near-invisible so it reads as absent data. */
	inline constexpr FLinearColor GradeUnknown{0.0685f, 0.0908f, 0.1170f, 1.0f};   // #4A5560

	/** Corrupted sits off the ladder entirely - nothing else in the menu is magenta. */
	inline constexpr FLinearColor GradeCorrupted{0.5271f, 0.0194f, 0.6514f, 1.0f}; // #C026D3
}
