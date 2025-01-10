// Copyright@2024 Quentin Davis 

#include "Character/Player/PHPlayerCharacter.h"

#include "AbilitySystem/PHAbilitySystemComponent.h"
#include "Character/ALSPlayerController.h"
#include "Character/Player/State/PHPlayerState.h"
#include "Components/InventoryManager.h"
#include "Kismet/KismetInputLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/HUD/PHHUD.h"

APHPlayerCharacter::APHPlayerCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}