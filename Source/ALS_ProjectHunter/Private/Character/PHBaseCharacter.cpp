// Copyright@2024 Quentin Davis 

#include "Character/PHBaseCharacter.h"

#include "PHGameplayTags.h"
#include "AbilitySystem/PHAbilitySystemComponent.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Character/ALSPlayerController.h"
#include "Character/Player/PHPlayerController.h"
#include "Character/Player/State/PHPlayerState.h"
#include "Components/EquipmentManager.h"
#include "Components/InventoryManager.h"
#include "Components/CombatManager.h"
#include "Interactables/Pickups/ConsumablePickup.h"
#include "Interactables/Pickups/EquipmentPickup.h"
#include "UI/ToolTip/ConsumableToolTip.h"
#include "UI/HUD/PHHUD.h"

class APHPlayerState;

/* =========================== */
/* === Constructor & Setup === */
/* =========================== */



APHBaseCharacter::APHBaseCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	EquipmentManager = CreateDefaultSubobject<UEquipmentManager>(TEXT("EquipmentManager"));
	InventoryManager = CreateDefaultSubobject<UInventoryManager>(TEXT("InventoryManager"));
	CombatManager = CreateDefaultSubobject<UCombatManager>(TEXT("Combat Manager"));
	StatsManager = CreateDefaultSubobject<UStatsManager>(TEXT("Stats Manager"));
	

	// GAS Setup
	if (!bIsPlayer)
	{
		AbilitySystemComponent = CreateDefaultSubobject<UPHAbilitySystemComponent>("Ability System Component");
		AbilitySystemComponent->SetIsReplicated(true);
		AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
		AttributeSet = CreateDefaultSubobject<UPHAttributeSet>("Attribute Set");
		
	}
	
}

void APHBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}


void APHBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	StatsManager->Owner = this; 
	InitAbilityActorInfo();
	
	CurrentController = Cast<APHPlayerController>(NewController);
	if (CurrentController)
	{
		CurrentController->InteractionManager->OnRemoveCurrentInteractable.AddDynamic(this, &APHBaseCharacter::CloseToolTip);
		CurrentController->InteractionManager->OnNewInteractableAssigned.AddDynamic(this, &APHBaseCharacter::OpenToolTip);
	}
}

UAbilitySystemComponent* APHBaseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APHBaseCharacter::StartSprintStaminaDrain()
{
	if (!GetAbilitySystemComponent() || !SprintStaminaDrainEffectClass)
	{
		return;
	}
    
	// Remove any existing sprint drain effect
	if (ActiveSprintDrainHandle.IsValid())
	{
		GetAbilitySystemComponent()->RemoveActiveGameplayEffect(ActiveSprintDrainHandle);
	}
    
	// Apply a new sprint drain effect
	FGameplayEffectContextHandle Context = GetAbilitySystemComponent()->MakeEffectContext();
	Context.AddSourceObject(this);
    
	FGameplayEffectSpecHandle Spec = GetAbilitySystemComponent()->MakeOutgoingSpec(
		SprintStaminaDrainEffectClass, 1.0f, Context);
    
	if (Spec.IsValid())
	{
		ActiveSprintDrainHandle = GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
        
		// Add tags
		GetAbilitySystemComponent()->AddLooseGameplayTag(
			FGameplayTag::RequestGameplayTag(TEXT("Condition.State.Sprinting")));
		GetAbilitySystemComponent()->AddLooseGameplayTag(
			FGameplayTag::RequestGameplayTag(TEXT("Effect.Stamina.DegenActive")));
        
		// Start checking stamina periodically
		if (bStopSprintingWhenStaminaDepleted)
		{
			GetWorldTimerManager().SetTimer(StaminaCheckTimer, this, 
				&APHBaseCharacter::CheckStaminaForSprint, 0.1f, true);
		}
        
		UE_LOG(LogTemp, Log, TEXT("Started sprint stamina drain"));
	}

	if (UPHAttributeSet* Attributes = Cast<UPHAttributeSet>(GetAttributeSet()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Stamina before drain: %f"), Attributes->GetStamina());
	}
}

void APHBaseCharacter::StopSprintStaminaDrain()
{
	if (!GetAbilitySystemComponent())
	{
		return;
	}
    
	// Remove sprint drain effect
	if (ActiveSprintDrainHandle.IsValid())
	{
		GetAbilitySystemComponent()->RemoveActiveGameplayEffect(ActiveSprintDrainHandle);
		ActiveSprintDrainHandle.Invalidate();
	}
    
	// Remove tags
	GetAbilitySystemComponent()->RemoveLooseGameplayTag(
		FGameplayTag::RequestGameplayTag(TEXT("Condition.State.Sprinting")));
	GetAbilitySystemComponent()->RemoveLooseGameplayTag(
		FGameplayTag::RequestGameplayTag(TEXT("Effect.Stamina.DegenActive")));
    
	// Clear stamina check timer
	GetWorldTimerManager().ClearTimer(StaminaCheckTimer);
    
	UE_LOG(LogTemp, Log, TEXT("Stopped sprint stamina drain"));
}

bool APHBaseCharacter::CanStartSprinting() const
{
	if (UPHAttributeSet* Attributes = Cast<UPHAttributeSet>(GetAttributeSet()))
	{
		return Attributes->GetStamina() >= MinimumStaminaToStartSprint;
	}
    
	return false;
}

void APHBaseCharacter::CheckStaminaForSprint()
{
	if (!bStopSprintingWhenStaminaDepleted)
	{
		return;
	}
    
	if (UPHAttributeSet* Attributes = Cast<UPHAttributeSet>(GetAttributeSet()))
	{
		if (Attributes->GetStamina() <= 0.1f) // Near zero
		{
			// Force stop sprinting
			SetDesiredGait(EALSGait::Running);
			
			OnStaminaDepleted();
            
			UE_LOG(LogTemp, Warning, TEXT("Stamina depleted - stopping sprint"));
		}
	}
}

void APHBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void APHBaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}


void APHBaseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const bool bHasMovingTag = ASC->HasMatchingGameplayTag(FPHGameplayTags::Get().Condition_WhileMoving);

		if (bIsMoving && !bHasMovingTag)
			ASC->AddLooseGameplayTag(FPHGameplayTags::Get().Condition_WhileMoving);
		else if (!bIsMoving && bHasMovingTag)
			ASC->RemoveLooseGameplayTag(FPHGameplayTags::Get().Condition_WhileMoving);

		const bool bHasStationaryTag = ASC->HasMatchingGameplayTag(FPHGameplayTags::Get().Condition_WhileStationary);

		if (!bIsMoving && !bHasStationaryTag)
			ASC->AddLooseGameplayTag(FPHGameplayTags::Get().Condition_WhileStationary);
		else if (bIsMoving && bHasStationaryTag)
			ASC->RemoveLooseGameplayTag(FPHGameplayTags::Get().Condition_WhileStationary);
	}	
}

bool APHBaseCharacter::CanSprint() const
{
	const bool bReturnValue =  Super::CanSprint();
	
	if(bIsInRecovery)
	{
		return false;
	}
	return bReturnValue;
}

void APHBaseCharacter::OnGaitChanged(EALSGait PreviousGait)
{
	
	Super::OnGaitChanged(PreviousGait);
	
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	const FGameplayTag& StaminaDegenTag = FPHGameplayTags::Get().Effect_Stamina_DegenActive;

	if (Gait == EALSGait::Sprinting)
	{
		ASC->AddLooseGameplayTag(StaminaDegenTag);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(StaminaDegenTag);
	}

	if (Gait == EALSGait::Sprinting && PreviousGait != EALSGait::Sprinting)
	{
		StartSprintStaminaDrain();
	}
	else if (Gait != EALSGait::Sprinting && PreviousGait == EALSGait::Sprinting)
	{
		StopSprintStaminaDrain();
	}
}


/* =========================== */
/* === Ability System (GAS) === */
/* =========================== */

void APHBaseCharacter::InitAbilityActorInfo()
{
	if (!bIsPlayer)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		Cast<UPHAbilitySystemComponent>(AbilitySystemComponent)->AbilityActorInfoSet();
		
		StatsManager->ASC = Cast<UPHAbilitySystemComponent>(AbilitySystemComponent);
		StatsManager->InitializeDefaultAttributes(); 
	}
	else
	{
		APHPlayerState* LocalPlayerState = GetPlayerState<APHPlayerState>();
		check(LocalPlayerState);

		
		AbilitySystemComponent = LocalPlayerState->GetAbilitySystemComponent();
		AbilitySystemComponent->InitAbilityActorInfo(LocalPlayerState, this);
		Cast<UPHAbilitySystemComponent>(AbilitySystemComponent)->AbilityActorInfoSet();
		UPHAbilitySystemComponent* ASC =  Cast<UPHAbilitySystemComponent>(AbilitySystemComponent);
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(this);
		StatsManager->ASC = ASC;
		
		StatsManager->InitializeDefaultAttributes();
		
		AttributeSet = LocalPlayerState->GetAttributeSet();

		if (AALSPlayerController* PHPlayerController = Cast<AALSPlayerController>(GetController()))
		{
			if (APHHUD* LocalPHHUD = Cast<APHHUD>(PHPlayerController->GetHUD()))
			{
				LocalPHHUD->InitOverlay(PHPlayerController, GetPlayerState<APHPlayerState>(), AbilitySystemComponent, AttributeSet);
			}
		}
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag(TEXT("Condition.State.Dead")),
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &APHBaseCharacter::OnGameplayTagChanged);
	}
}

/* =========================== */
/* === UI & MiniMap === */
/* =========================== */

int32 APHBaseCharacter::GetPlayerLevel()
{
	if(bIsPlayer)
	{
		const APHPlayerState* LocalPlayerState = GetPlayerState<APHPlayerState>();
		check(LocalPlayerState)
		return  LocalPlayerState->GetPlayerLevel();
	}

	return Level;
}

int32 APHBaseCharacter::GetCurrentXP()
{
		const APHPlayerState* LocalPlayerState = GetPlayerState<APHPlayerState>();
		check(LocalPlayerState)
		return  LocalPlayerState->GetXP();
}

int32 APHBaseCharacter::GetXPFNeededForNextLevel()
{
	const APHPlayerState* LocalPlayerState = GetPlayerState<APHPlayerState>();
	check(LocalPlayerState)
	return  LocalPlayerState->GetXPForNextLevel();
}

void APHBaseCharacter::OpenToolTip(UInteractableManager* InteractableManager)
{
	if (!InteractableManager) return;
	check(EquippableToolTipClass);
	
	AItemPickup* PickupItem = Cast<AItemPickup>(InteractableManager->GetOwner());
	if (!PickupItem) return;

	if (CurrentToolTip)
	{
		CurrentToolTip->RemoveFromParent();
		CurrentToolTip = nullptr;
	}

	// Use the Tooltip Widget as the Outer to keep the item alive
	UUserWidget* ToolTipWidget = nullptr;

	if (Cast<AEquipmentPickup>(PickupItem) && EquippableToolTipClass)
	{
		ToolTipWidget = CreateWidget<UEquippableToolTip>(GetWorld(), EquippableToolTipClass);
		
	}
	else if (Cast<AConsumablePickup>(PickupItem) && ConsumableToolTipClass)
	{
		ToolTipWidget = CreateWidget<UConsumableToolTip>(GetWorld(), ConsumableToolTipClass);
	}

	if (!ToolTipWidget) return;

	// Create the item instance using the widget as the Outer to avoid GC issues
	UBaseItem* ItemObject = PickupItem->CreateItemObject(GetTransientPackage());
	if (!ItemObject) return;

	ItemObject->SetItemInfo(PickupItem->ItemInfo);
	ItemObject->SetRotated(PickupItem->ItemInfo.ItemInfo.Rotated);

	// Cast and assign
	if (UPHBaseToolTip* ToolTip = Cast<UPHBaseToolTip>(ToolTipWidget))
	{
		if(UEquippableToolTip* EquipToolTip = Cast<UEquippableToolTip>(ToolTip))
		{
			EquipToolTip->SetItemInfo(ItemObject->GetItemInfo());
			EquipToolTip->OwnerCharacter = this;
			EquipToolTip->AddToViewport();
			CurrentToolTip = EquipToolTip;
		}
		else
		{
			ToolTip->SetItemInfo(ItemObject->GetItemInfo());
			ToolTip->AddToViewport();
			CurrentToolTip = ToolTip;
		}
	}
}

void APHBaseCharacter::CloseToolTip(UInteractableManager* InteractableManager)
{
	if (IsValid(CurrentToolTip))
	{
		CurrentToolTip->RemoveFromParent();
	}
}

void APHBaseCharacter::OnGameplayTagChanged(FGameplayTag Tag, int NewCount)
{
	// Check if the dead tag was added (NewCount > 0 means the tag was added)
	if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Condition.State.Dead"))) && NewCount > 0)
	{
		if (!bHasHandledDeath)
		{
			HandleDeathState();
		}
	}
}
void APHBaseCharacter::HandleDeathState()
{
	if (bHasHandledDeath)
	{
		return;
	}

	bHasHandledDeath = true;

	UE_LOG(LogTemp, Warning, TEXT("%s: Handling death state - destroying all components"), *GetName());

	// Call the component destruction
	DestroyAllComponents();

	// Add any additional death handling logic here,
	// For example, play death animation, disable collision, etc.
}

void APHBaseCharacter::DestroyAllComponents()
{

	// Check for and destroy the InteractableManager component
	if (UInteractableManager* InteractableManager = GetComponentByClass<UInteractableManager>())
	{
		InteractableManager->ToggleInteractionWidget(false);
		InteractableManager->DestroyComponent();
		UE_LOG(LogTemp, Log, TEXT("InteractableManager destroyed"));
	}
	
	// Destroy Equipment Manager
	if (IsValid(EquipmentManager))
	{
		EquipmentManager->DestroyComponent();
		EquipmentManager = nullptr;
		UE_LOG(LogTemp, Log, TEXT("EquipmentManager destroyed"));
	}

	// Destroy Inventory Manager
	if (IsValid(InventoryManager))
	{
		InventoryManager->DestroyComponent();
		InventoryManager = nullptr;
		UE_LOG(LogTemp, Log, TEXT("InventoryManager destroyed"));
	}

	// Destroy Combat Manager
	if (IsValid(CombatManager))
	{
		CombatManager->DestroyComponent();
		CombatManager = nullptr;
		UE_LOG(LogTemp, Log, TEXT("CombatManager destroyed"));
	}

	// Destroy Stats Manager
	if (IsValid(StatsManager))
	{
		StatsManager->DestroyComponent();
		StatsManager = nullptr;
		UE_LOG(LogTemp, Log, TEXT("StatsManager destroyed"));
	}

	// Close any open tooltips
	if (IsValid(CurrentToolTip))
	{
		CurrentToolTip->RemoveFromParent();
		CurrentToolTip = nullptr;
	}

	// Clear any active timers
	GetWorldTimerManager().ClearTimer(StaminaCheckTimer);

	// Remove any active gameplay effects
	if (ActiveSprintDrainHandle.IsValid() && AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveSprintDrainHandle);
		ActiveSprintDrainHandle.Invalidate();
	}

	UE_LOG(LogTemp, Warning, TEXT("%s: All components destroyed due to death"), *GetName());
}