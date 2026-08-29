// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "PHMenuCameraTypes.generated.h"

/**
 * One framing of the character for the menu camera.
 *
 * Every value is relative to the character's ALS pivot (the midpoint between
 * the head and root sockets), so a view stays correct whatever the character
 * is standing on and wherever in the world the menu was opened.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHMenuCameraView
{
	GENERATED_BODY()

	/** Horizontal distance from the pivot. Low is a portrait, high is full body. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Camera",
		meta = (ClampMin = "30.0", UIMin = "80.0", UIMax = "600.0"))
	float Distance = 300.0f;

	/** Raises the point the camera looks at. Positive frames the upper body. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Camera",
		meta = (UIMin = "-100.0", UIMax = "150.0"))
	float PivotHeightOffset = 60.0f;

	/**
	 * Degrees around the character, measured from directly in front of them.
	 * 35-45 is the three-quarter view that reads best. 0 is a flat mugshot.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Camera",
		meta = (ClampMin = "-180.0", ClampMax = "180.0"))
	float YawOffset = 40.0f;

	/** Camera pitch. Negative looks up at the character, which reads heroic. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Camera",
		meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float Pitch = -4.0f;

	/**
	 * Slides the camera sideways without turning it, pushing the character off
	 * centre. Positive moves the character towards the screen's left and frees
	 * the right-hand side for the menu panel. Negative mirrors it.
	 *
	 * Zero by default because the equipment page already reserves a centre
	 * column for the character; raise it for a layout that puts the panels on
	 * one side only.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Camera",
		meta = (UIMin = "-200.0", UIMax = "200.0"))
	float LateralOffset = 0.0f;

	/**
	 * Long lens flatters, wide lens distorts faces and inflates weapons.
	 * ALS gameplay runs at 90, so anything near that will look wrong here.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Camera",
		meta = (ClampMin = "5.0", ClampMax = "120.0"))
	float FOV = 40.0f;
};

/**
 * A delta applied on top of the active view while one equipment slot is
 * focused, so hovering the helmet slot drifts the camera up to the head.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHMenuCameraFocus
{
	GENERATED_BODY()

	FPHMenuCameraFocus() = default;

	FPHMenuCameraFocus(const float InHeight, const float InDistanceScale, const float InYawOffset = 0.0f)
		: PivotHeightOffset(InHeight)
		, DistanceScale(InDistanceScale)
		, YawOffset(InYawOffset)
	{
	}

	/** Added to the view's PivotHeightOffset. Positive moves framing up the body. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Camera Focus",
		meta = (UIMin = "-100.0", UIMax = "150.0"))
	float PivotHeightOffset = 0.0f;

	/** Multiplies the view's Distance. Below 1 pushes in on the part. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Camera Focus",
		meta = (ClampMin = "0.1", UIMin = "0.3", UIMax = "1.5"))
	float DistanceScale = 1.0f;

	/** Added to the view's YawOffset, for parts that read better from one side. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Camera Focus",
		meta = (ClampMin = "-180.0", ClampMax = "180.0"))
	float YawOffset = 0.0f;
};
