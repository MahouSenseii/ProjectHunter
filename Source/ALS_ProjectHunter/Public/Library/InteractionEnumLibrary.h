// Copyright@2024 Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "InteractionEnumLibrary.generated.h"

UENUM(BlueprintType)
enum class EInteractableResponseType : uint8
{
	Persistent UMETA(DisplayName = "Persistent"),
	OnlyOnce UMETA(DisplayName = "Only Once"),
	Temporary UMETA(DisplayName = "Temporary"),
};

UENUM(BlueprintType)
enum class EInteractType : uint8
{
	Single UMETA(DisplayName = "Single"),
	Holding UMETA(DisplayName = "Holding"),
	Mashing UMETA(DisplayName = "Mashing"),
};

UENUM(BlueprintType)
enum class ERotation : uint8
{
	X,
	Y,
	Z
};


