#include "Item/EquippedObject.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CombatManager.h"
#include "Character/PHBaseCharacter.h"
#include "Library/PHItemFunctionLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

#define LOCTEXT_NAMESPACE "EquippedObject"

AEquippedObject::AEquippedObject()
{
    PrimaryActorTick.bCanEverTick = false;
    
    // Create components
    DefaultScene = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultScene"));
    RootComponent = DefaultScene;
    
    StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    StaticMesh->SetupAttachment(DefaultScene);
    StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
    SkeletalMesh->SetupAttachment(DefaultScene);
    SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    DamageSpline = CreateDefaultSubobject<USplineComponent>(TEXT("DamageSpline"));
    DamageSpline->SetupAttachment(DefaultScene);
}

void AEquippedObject::BeginPlay()
{
    Super::BeginPlay();
    
    if (bIsInitialized)
    {
        UpdateVisualEffects();
    }
}

// === Initialization ===

void AEquippedObject::InitializeFromItem(const UItemDefinitionAsset* InDefinition, const FItemInstanceData& InInstanceData)
{
    if (!InDefinition)
    {
        UE_LOG(LogTemp, Warning, TEXT("EquippedObject: Cannot initialize with null definition"));
        return;
    }
    
    
    ItemDefinition = InDefinition;
    InstanceData = InInstanceData;
    bIsInitialized = true;
    
    // Setup visual representation
    SetupMesh();
    UpdateVisualEffects();
    
    UE_LOG(LogTemp, Log, TEXT("EquippedObject initialized: %s"),
           *GetDisplayName().ToString());
}

void AEquippedObject::UpdateInstanceData(const FItemInstanceData& NewInstanceData)
{
    if (!bIsInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("EquippedObject: Cannot update instance data before initialization"));
        return;
    }
    
    // ✅ Update instance data (e.g., after crafting operations)
    InstanceData = NewInstanceData;
    
    // Update visuals to reflect changes
    UpdateVisualEffects();
    
    // Broadcast update event
    OnInstanceDataUpdated.Broadcast();
    
    UE_LOG(LogTemp, Log, TEXT("EquippedObject: Instance data updated"));
}

void AEquippedObject::SetupMesh()
{
    if (!ItemDefinition) return;
    
    // ✅ Use static mesh from definition
    if (ItemDefinition->Base.StaticMesh && StaticMesh)
    {
        StaticMesh->SetStaticMesh(ItemDefinition->Base.StaticMesh);
        StaticMesh->SetVisibility(true);
        
        if (SkeletalMesh)
        {
            SkeletalMesh->SetVisibility(false);
        }
    }
    // ✅ Or use skeletal mesh from definition
    else if (ItemDefinition->Base.SkeletalMesh && SkeletalMesh)
    {
        SkeletalMesh->SetSkeletalMesh(ItemDefinition->Base.SkeletalMesh);
        SkeletalMesh->SetVisibility(true);
        
        if (StaticMesh)
        {
            StaticMesh->SetVisibility(false);
        }
    }
}

void AEquippedObject::UpdateVisualEffects()
{
    if (!bIsInitialized) return;
    
    // Apply visual effects based on rarity
    const EItemRarity Rarity = GetRarity();
    
    switch (Rarity)
    {
    case EItemRarity::IR_GradeS:
        // Legendary glow, particles, etc.
        break;
    case EItemRarity::IR_GradeA:
        // Epic glow
        break;
    case EItemRarity::IR_GradeB:
        // Rare glow
        break;
    default:
        // No special effects
        break;
    }
    
    // Could also show visual indicators for:
    // - Applied runes (glowing symbols)
    // - Number of affixes (aura intensity)
    // - Low durability warning (for crafting UI)
}

// === IEquippable Interface Implementation ===

void AEquippedObject::OnEquipped_Implementation(AActor* NewOwner)
{
    if (!NewOwner)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnEquipped: NewOwner is null"));
        return;
    }
    
    if (!bIsInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnEquipped: Item not initialized"));
        return;
    }
    
    OwningCharacter = Cast<AALSBaseCharacter>(NewOwner);
    bIsEquipped = true;
    
    // Apply stats to owner
    ApplyStatsToOwner(NewOwner);
    
    // Attach to owner
    if (USkeletalMeshComponent* OwnerMesh = NewOwner->FindComponentByClass<USkeletalMeshComponent>())
    {
        const FName Socket = GetAttachmentSocket_Implementation();
        
        if (ItemDefinition && !ItemDefinition->Base.AttachmentRules.AttachmentOffset.Equals(FTransform::Identity))
        {
            // Use custom attachment rules if specified
            AttachToComponent(OwnerMesh, ItemDefinition->Base.AttachmentRules.ToAttachmentRules(), Socket);
            SetActorRelativeTransform(ItemDefinition->Base.AttachmentRules.AttachmentOffset);
        }
        else
        {
            // Default attachment
            AttachToComponent(OwnerMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
        }
    }
    
    // Broadcast event
    OnEquipped.Broadcast(this, NewOwner);
    
    UE_LOG(LogTemp, Log, TEXT("Item equipped: %s"), *GetDisplayName().ToString());
}

void AEquippedObject::OnUnequipped_Implementation()
{
    if (OwningCharacter)
    {
        RemoveStatsFromOwner(OwningCharacter);
    }
    
    // Detach from owner
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    
    OwningCharacter = nullptr;
    bIsEquipped = false;
    
    // Broadcast event
    OnUnequipped.Broadcast(this);
    
    UE_LOG(LogTemp, Log, TEXT("Item unequipped: %s"), *GetDisplayName().ToString());
}

bool AEquippedObject::CanBeEquippedBy_Implementation(AActor* Actor) const
{
    if (!Actor || !bIsInitialized)
    {
        return false;
    }
    
    // Check stat requirements if it's a character
    if (const APHBaseCharacter* Character = Cast<APHBaseCharacter>(Actor))
    {
        if (ItemDefinition)
        {
            // Check if character meets requirements
            return ItemDefinition->Equip.MeetsRequirements(Character->GetCurrentStats());
        }
    }
    
    return true;
}

FName AEquippedObject::GetAttachmentSocket_Implementation() const
{
    if (!ItemDefinition)
    {
        return DefaultAttachmentSocket;
    }
    
    // Check if item has a specific socket defined
    if (!ItemDefinition->Base.AttachmentSocket.IsNone())
    {
        return ItemDefinition->Base.AttachmentSocket;
    }
    
    // Return socket based on equipment slot
    return UPHItemFunctionLibrary::GetSocketNameForSlot(ItemDefinition->Base.EquipmentSlot);
}

UItemDefinitionAsset* AEquippedObject::GetEquippableItemInfo_Implementation() const
{
    // Note: Casting away const here - this is a legacy interface issue
    // In future, this should return const UItemDefinitionAsset*
    return const_cast<UItemDefinitionAsset*>(ItemDefinition);
}

void AEquippedObject::ApplyStatsToOwner_Implementation(AActor* RefOwner)
{
    if (!RefOwner || !bIsInitialized)
    {
        return;
    }
    
    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(RefOwner);
    if (!ASC)
    {
        return;
    }
    
    // Clear any previously applied effects
    for (const FActiveGameplayEffectHandle& Handle : AppliedEffectHandles)
    {
        if (Handle.IsValid())
        {
            ASC->RemoveActiveGameplayEffect(Handle);
        }
    }
    AppliedEffectHandles.Empty();
    
    // Apply passive effects from definition
    for (const FItemPassiveEffectInfo& PassiveEffect : ItemDefinition->Equip.PassiveEffects)
    {
        if (PassiveEffect.EffectClass && PassiveEffect.bIsActive)
        {
            FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
            EffectContext.AddSourceObject(this);
            
            FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
                PassiveEffect.EffectClass,
                PassiveEffect.Level,
                EffectContext
            );
            
            if (SpecHandle.IsValid())
            {
                FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
                if (ActiveHandle.IsValid())
                {
                    AppliedEffectHandles.Add(ActiveHandle);
                }
            }
        }
    }
    
    // Apply stat modifiers from instance affixes
    // TODO: Create and apply gameplay effects based on affixes
    // This would require a system to convert FPHAttributeData to gameplay effects
    
    UE_LOG(LogTemp, Log, TEXT("Applied %d effects to owner"), AppliedEffectHandles.Num());
}

void AEquippedObject::RemoveStatsFromOwner_Implementation(AActor* RefOwner)
{
    if (!RefOwner)
    {
        return;
    }
    
    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(RefOwner);
    if (!ASC)
    {
        return;
    }
    
    // Remove all applied effects
    for (const FActiveGameplayEffectHandle& Handle : AppliedEffectHandles)
    {
        if (Handle.IsValid())
        {
            ASC->RemoveActiveGameplayEffect(Handle);
        }
    }
    
    AppliedEffectHandles.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("Removed effects from owner"));
}

EEquipmentSlot AEquippedObject::GetEquipmentSlot_Implementation() const
{
    return ItemDefinition ? ItemDefinition->Base.EquipmentSlot : EEquipmentSlot::ES_None;
}

// === Getters ===

FText AEquippedObject::GetDisplayName() const
{
    if (!bIsInitialized) return FText::GetEmpty();
    
    // Use instance name if generated
    if (InstanceData.bHasNameBeenGenerated)
    {
        return InstanceData.DisplayName;
    }
    
    // Fall back to definition name
    return ItemDefinition ? ItemDefinition->Base.ItemName : FText::GetEmpty();
}

EItemRarity AEquippedObject::GetRarity() const
{
    if (!bIsInitialized) return EItemRarity::IR_None;
    
    // Use instance rarity if set
    if (InstanceData.Rarity != EItemRarity::IR_None)
    {
        return InstanceData.Rarity;
    }
    
    // Fall back to definition rarity
    return ItemDefinition ? ItemDefinition->Base.ItemRarity : EItemRarity::IR_None;
}

// === Utility ===

UCombatManager* AEquippedObject::ConvertActorToCombatManager(const AActor* InActor)
{
    return InActor ? InActor->FindComponentByClass<UCombatManager>() : nullptr;
}

bool AEquippedObject::IsValidForEquipping() const
{
    return bIsInitialized && ItemDefinition && !GetDisplayName().IsEmpty();
}

// === Pooling Support ===

void AEquippedObject::ResetForPool()
{
    // Remove effects if equipped
    if (bIsEquipped && OwningCharacter)
    {
        RemoveStatsFromOwner(OwningCharacter);
    }
    
    // Reset all state
    bIsEquipped = false;
    bIsInitialized = false;
    bIsFromPool = true;
    OwningCharacter = nullptr;
    ItemDefinition = nullptr;
    InstanceData = FItemInstanceData();
    CurrentActorHit.Empty();
    PrevTracePoints.Empty();
    AppliedEffectHandles.Empty();
    
    // Detach from any parent
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    
    // Hide the actor
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
    
    // Clear meshes
    if (StaticMesh)
    {
        StaticMesh->SetStaticMesh(nullptr);
        StaticMesh->SetVisibility(false);
    }
    
    if (SkeletalMesh)
    {
        SkeletalMesh->SetSkeletalMesh(nullptr);
        SkeletalMesh->SetVisibility(false);
    }
}

void AEquippedObject::ReinitializeFromPool(const UItemDefinitionAsset* InDefinition, const FItemInstanceData& InInstanceData)
{
    // Initialize with new data
    InitializeFromItem(InDefinition, InInstanceData);
    
    // Show the actor
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    SetActorTickEnabled(PrimaryActorTick.bStartWithTickEnabled);
    
    bIsFromPool = true;
}

#undef LOCTEXT_NAMESPACE