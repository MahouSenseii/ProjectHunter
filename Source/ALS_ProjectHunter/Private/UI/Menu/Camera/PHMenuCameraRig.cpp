// Author: Quentin Davis

#include "UI/Menu/Camera/PHMenuCameraRig.h"

#include "Camera/CameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Scene.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/**
	 * Light positions are fixed rather than scaled off the camera distance:
	 * lighting that follows the zoom changes the character's read every time
	 * the player scrolls, which is exactly what a studio setup avoids.
	 */
	constexpr float KeyLightRelativeX = -130.0f;
	constexpr float KeyLightRelativeY = -155.0f;
	constexpr float KeyLightRelativeZ = 125.0f;

	constexpr float FillLightRelativeX = -145.0f;
	constexpr float FillLightRelativeY = 160.0f;
	constexpr float FillLightRelativeZ = 30.0f;

	constexpr float RimLightRelativeX = 165.0f;
	constexpr float RimLightRelativeY = 120.0f;
	constexpr float RimLightRelativeZ = 165.0f;

	/** Rotation that aims a component sitting at RelativeLocation back at the pivot. */
	FRotator MakeAimAtPivotRotation(const FVector& RelativeLocation)
	{
		return (-RelativeLocation).Rotation();
	}
}

APHMenuCameraRig::APHMenuCameraRig()
{
	PrimaryActorTick.bCanEverTick = false;

	// The menu drives this actor's transform directly every frame.
	SetActorEnableCollision(false);

	PivotRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PivotRoot"));
	SetRootComponent(PivotRoot);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(PivotRoot);
	Camera->SetRelativeLocation(FVector(-230.0f, 0.0f, 0.0f));
	Camera->bConstrainAspectRatio = false;

	const FVector KeyLocation(KeyLightRelativeX, KeyLightRelativeY, KeyLightRelativeZ);
	KeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(PivotRoot);
	KeyLight->SetRelativeLocationAndRotation(KeyLocation, MakeAimAtPivotRotation(KeyLocation));

	const FVector FillLocation(FillLightRelativeX, FillLightRelativeY, FillLightRelativeZ);
	FillLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(PivotRoot);
	FillLight->SetRelativeLocationAndRotation(FillLocation, MakeAimAtPivotRotation(FillLocation));

	const FVector RimLocation(RimLightRelativeX, RimLightRelativeY, RimLightRelativeZ);
	RimLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("RimLight"));
	RimLight->SetupAttachment(PivotRoot);
	RimLight->SetRelativeLocationAndRotation(RimLocation, MakeAimAtPivotRotation(RimLocation));

	Backdrop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Backdrop"));
	Backdrop->SetupAttachment(PivotRoot);
	// FRotator(90,0,0) maps the plane's +Z normal onto -X, which is where the camera is.
	Backdrop->SetRelativeLocationAndRotation(
		FVector(BackdropDistance, 0.0f, 0.0f), FRotator(90.0f, 0.0f, 0.0f));
	Backdrop->SetRelativeScale3D(FVector(BackdropScale));
	Backdrop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Backdrop->SetCastShadow(false);
	Backdrop->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	BackdropMesh = PlaneMesh.Succeeded() ? PlaneMesh.Object : nullptr;

	// Created hidden. The menu camera component reveals the rig on activation.
	SetActorHiddenInGame(true);
}

void APHMenuCameraRig::BeginPlay()
{
	Super::BeginPlay();

	ApplyLightingSettings();

	Backdrop->SetRelativeLocation(FVector(BackdropDistance, 0.0f, 0.0f));
	Backdrop->SetRelativeScale3D(FVector(BackdropScale));

	// Assigned only alongside a material: the plane's default material is an
	// opaque grey card the size of the whole view, which is far worse than
	// whatever the player was standing in front of.
	if (BackdropMaterial && BackdropMesh)
	{
		Backdrop->SetStaticMesh(BackdropMesh);
		Backdrop->SetMaterial(0, BackdropMaterial);
	}

	SetRigVisible(false);
}

void APHMenuCameraRig::ApplyLightingSettings()
{
	const auto ConfigureLight = [](USpotLightComponent* Light, const float Intensity,
		const FLinearColor& Color, const float OuterCone, const bool bCastShadows)
	{
		if (!Light)
		{
			return;
		}

		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(Intensity);
		Light->SetLightColor(Color);
		Light->SetInnerConeAngle(FMath::Max(0.0f, OuterCone - 18.0f));
		Light->SetOuterConeAngle(OuterCone);
		Light->SetAttenuationRadius(1200.0f);
		Light->SetCastShadows(bCastShadows);
		Light->SetVisibility(false);
	};

	// Only the key light casts shadows - fill and rim exist to lift and separate,
	// and shadowing them just costs frames and muddies the silhouette.
	ConfigureLight(KeyLight, KeyLightIntensity, KeyLightColor, 50.0f, true);
	ConfigureLight(FillLight, FillLightIntensity, FillLightColor, 60.0f, false);
	ConfigureLight(RimLight, RimLightIntensity, RimLightColor, 44.0f, false);
}

void APHMenuCameraRig::ApplyCameraOffset(const float Distance, const float LateralOffset,
	const float HeightOffset, const float Pitch, const float FOV)
{
	if (!Camera)
	{
		return;
	}

	Camera->SetRelativeLocation(FVector(-Distance, LateralOffset, HeightOffset));
	Camera->SetRelativeRotation(FRotator(Pitch, 0.0f, 0.0f));
	Camera->SetFieldOfView(FOV);
}

void APHMenuCameraRig::ApplyDepthOfField(const float FocalDistance)
{
	if (!Camera)
	{
		return;
	}

	FPostProcessSettings& Settings = Camera->PostProcessSettings;

	Settings.bOverride_DepthOfFieldFocalDistance = bUseDepthOfField;
	Settings.bOverride_DepthOfFieldFstop = bUseDepthOfField;
	Settings.bOverride_VignetteIntensity = true;

	if (bUseDepthOfField)
	{
		Settings.DepthOfFieldFocalDistance = FMath::Max(1.0f, FocalDistance);
		Settings.DepthOfFieldFstop = DepthOfFieldFstop;
	}

	Settings.VignetteIntensity = VignetteIntensity;
}

void APHMenuCameraRig::SetStudioLightingVisible(const bool bNewVisible)
{
	const bool bShow = bNewVisible && bUseStudioLighting;

	// Lights are toggled explicitly: SetActorHiddenInGame only reaches primitive
	// components, so a hidden rig would otherwise keep lighting the level.
	if (KeyLight)
	{
		KeyLight->SetVisibility(bShow);
	}
	if (FillLight)
	{
		FillLight->SetVisibility(bShow);
	}
	if (RimLight)
	{
		RimLight->SetVisibility(bShow);
	}
}

void APHMenuCameraRig::SetBackdropVisible(const bool bNewVisible)
{
	if (!Backdrop)
	{
		return;
	}

	// No material means the engine's grid material, which looks far worse than
	// whatever the player happened to be standing in front of.
	Backdrop->SetVisibility(bNewVisible && BackdropMaterial != nullptr);
}

void APHMenuCameraRig::SetRigVisible(const bool bNewVisible)
{
	SetActorHiddenInGame(!bNewVisible);
	SetStudioLightingVisible(bNewVisible);
	SetBackdropVisible(bNewVisible);
}
