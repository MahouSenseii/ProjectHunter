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
    
    DamageSpline = CreateDefaultSubobject<USplineComponent>(TEXT("DamageSpline"));
    DamageSpline->SetupAttachment(DefaultScene);
}

void AEquippedObject::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    
    
    // Generate stats after properties have been set from Blueprint/Editor
    if (!ItemInfo->ItemStats.bAffixesGenerated &&  ItemInfo->GetStatsDataTable())
    {
        ItemInfo->ItemStats = UPHItemFunctionLibrary::GenerateStats(ItemInfo->GetStatsDataTable());
        ItemInfo = UPHItemFunctionLibrary::GenerateItemName(ItemInfo->ItemStats, ItemInfo);
    }
    
    // Set initial durability
    CurrentDurability = MaxDurability;
    
    // Set static mesh if available
    if (StaticMesh && ItemInfo->Base.StaticMesh)
    {
        StaticMesh->SetStaticMesh(ItemInfo->Base.StaticMesh);
    }
}

void AEquippedObject::BeginPlay()
{
    Super::BeginPlay();
}

#if WITH_EDITOR
void AEquippedObject::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    if (PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();
        
        if (PropertyName == GET_MEMBER_NAME_CHECKED(AEquippedObject, ItemInfo))
        {
            FString ErrorMsg;
            if (!ValidateItemInfo(ItemInfo, ErrorMsg))
            {
                UE_LOG(LogTemp, Warning, TEXT("Invalid ItemInfo: %s"), *ErrorMsg);
            }
        }
    }
}
#endif

// === IEquippable Interface Implementation ===

void AEquippedObject::OnEquipped_Implementation(AActor* NewOwner)
{
    if (!NewOwner)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnEquipped: NewOwner is null"));
        return;
    }
    
    SetOwningCharacter(Cast<AALSBaseCharacter>(NewOwner));
    bIsEquipped = true;
    
    // Apply stats to owner
    ApplyStatsToOwner(NewOwner);
    
    // Attach to owner
    if (USkeletalMeshComponent* OwnerMesh = NewOwner->FindComponentByClass<USkeletalMeshComponent>())
    {
        AttachToComponent(OwnerMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, 
                         GetAttachmentSocket_Implementation());
    }
    
    // Broadcast event
    OnEquipped.Broadcast(this, NewOwner);
}

void AEquippedObject::OnUnequipped_Implementation()
{
    if (OwningCharacter)
    {
        RemoveStatsFromOwner(OwningCharacter);
    }
    
    // Detach from owner
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    
    AActor* PreviousOwner = OwningCharacter;
    OwningCharacter = nullptr;
    bIsEquipped = false;
    
    // Broadcast event
    OnUnequipped.Broadcast(this);
}

bool AEquippedObject::CanBeEquippedBy_Implementation(AActor* Actor) const
{
    if (!Actor || IsBroken())
    {
        return false;
    }
    
    // Check stat requirements if it's a character
    if (const APHBaseCharacter* Character = Cast<APHBaseCharacter>(Actor))
    {
        // TODO: Add stat requirement checks here
        // Example: return Character->MeetsRequirements(ItemInfo.ItemData.StatRequirements);
    }
    
    return true;
}

FName AEquippedObject::GetAttachmentSocket_Implementation() const
{
    // Check if item has a specific socket defined
    if (!ItemInfo->Base.AttachmentSocket.IsNone())
    {
        return ItemInfo->Base.AttachmentSocket;
    }
    
    // Return default socket based on item type
    switch (ItemInfo->Base.ItemType)
    {
        case EItemType::IT_Weapon:
            return DefaultAttachmentSocket;
        case EItemType::IT_Shield:
            return FName("ShieldSocket");
        case EItemType::IT_Armor:
            return FName("ArmorSocket");
        default:
            return DefaultAttachmentSocket;
    }
}

 UItemDefinitionAsset* AEquippedObject::GetEquippableItemInfo_Implementation() const
{
    return ItemInfo;
}

void AEquippedObject::ApplyStatsToOwner_Implementation(AActor* RefOnwer)
{
    if (!RefOnwer)
    {
        return;
    }
    
    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(RefOnwer);
    if (!ASC)
    {
        return;
    }
    
    // Apply each stat as a gameplay effect
    for (const FPHAttributeData& Stat : ItemInfo->ItemStats.GetAllStats())
    {
        // TODO: Apply stat modifiers to owner
        // This would require creating gameplay effects based on stats
        // Example: ASC->ApplyModToAttribute(Stat.AttributeName, EGameplayModOp::Additive, Stat.Value);
    }
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
    
    // Remove stat effects
    // This would require tracking applied effects and removing them
}

EEquipmentSlot AEquippedObject::GetEquipmentSlot_Implementation() const
{
    return ItemInfo->Base.EquipmentSlot;
}

// === Setters with Validation ===

bool AEquippedObject::SetItemInfo(UItemDefinitionAsset*& Info)
{
    FString ErrorMsg;
    if (!ValidateItemInfo(Info, ErrorMsg))
    {
        UE_LOG(LogTemp, Error, TEXT("SetItemInfo failed: %s"), *ErrorMsg);
        return false;
    }
    
    UItemDefinitionAsset*& OldInfo = ItemInfo;
    ItemInfo = Info;
    
    // Update mesh if needed
    if (StaticMesh && ItemInfo->Base.StaticMesh)
    {
        StaticMesh->SetStaticMesh(ItemInfo->Base.StaticMesh);
    }
    
    // Broadcast change event
    OnItemInfoChanged.Broadcast(OldInfo, ItemInfo);
    
    return true;
}

void AEquippedObject::SetItemInfoRotated(bool bRotated)
{
    ItemInfo->Base.Rotated = bRotated;
}

void AEquippedObject::SetMesh(UStaticMesh* Mesh) const
{
    ItemInfo->Base.StaticMesh = Mesh;
}

UCombatManager* AEquippedObject::ConvertActorToCombatManager(const AActor* InActor)
{
    return InActor ? InActor->FindComponentByClass<UCombatManager>() : nullptr;
}

bool AEquippedObject::SetOwningCharacter(AALSBaseCharacter* InOwner)
{
    if (InOwner == OwningCharacter)
    {
        return true; // No change needed
    }
    
    if (bIsEquipped && InOwner != nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot change owner while equipped"));
        return false;
    }
    
    OwningCharacter = InOwner;
    return true;
}

void AEquippedObject::SetDurability(float NewDurability)
{
    const float OldDurability = CurrentDurability;
    CurrentDurability = FMath::Clamp(NewDurability, 0.0f, MaxDurability);
    
    if (OldDurability != CurrentDurability)
    {
        OnDurabilityChanged.Broadcast(CurrentDurability);
        
        if (CurrentDurability <= 0.0f && OldDurability > 0.0f)
        {
            OnItemBroken.Broadcast(this);
        }
    }
}

void AEquippedObject::ModifyDurability(float DeltaDurability)
{
    SetDurability(CurrentDurability + DeltaDurability);
}

bool AEquippedObject::SetMesh(UStaticMesh* Mesh)
{
    if (!StaticMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("SetMesh: StaticMesh component is null"));
        return false;
    }
    
    if (!Mesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("SetMesh: Mesh parameter is null"));
        return false;
    }
    
    StaticMesh->SetStaticMesh(Mesh);
    return true;
}

// === Validation Functions ===

bool AEquippedObject::ValidateItemInfo( UItemDefinitionAsset*& Info, FString& OutError) const
{
    if (Info->Base.ItemName.IsEmpty())
    {
        OutError = TEXT("Item name cannot be empty");
        return false;
    }
    
    if (Info->Base.Dimensions.X <= 0 || Info->Base.Dimensions.Y <= 0)
    {
        OutError = FString::Printf(TEXT("Invalid dimensions: %dx%d"), 
                                   Info->Base.Dimensions.X, 
                                   Info->Base.Dimensions.Y);
        return false;
    }
    
    if (!Info->Base.ItemImage && !Info->Base.StaticMesh)
    {
        OutError = TEXT("Item must have either an icon or a mesh");
        return false;
    }
    
    if (Info->Base.MaxStackSize < 0)
    {
        OutError = TEXT("Max stack size cannot be negative");
        return false;
    }
    
    OutError.Empty();
    return true;
}

bool AEquippedObject::IsValidForEquipping() const
{
    return !IsBroken() && !ItemInfo->Base.ItemName.IsEmpty();
}

// === Pooling Support ===

void AEquippedObject::ResetForPool()
{
    // Reset all state
    bIsEquipped = false;
    bIsFromPool = true;
    OwningCharacter = nullptr;
    CurrentDurability = MaxDurability;
    CurrentActorHit.Empty();
    PrevTracePoints.Empty();
    
    // Detach from any parent
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    
    // Hide the actor
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
    
    // Clear mesh
    if (StaticMesh)
    {
        StaticMesh->SetStaticMesh(nullptr);
    }
}

void AEquippedObject::InitializeFromPool(UItemDefinitionAsset*& Info)
{
    // Set new item info
    SetItemInfo(Info);
    
    // Reset durability
    CurrentDurability = MaxDurability;
    
    // Show the actor
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    SetActorTickEnabled(PrimaryActorTick.bStartWithTickEnabled);
}

#undef LOCTEXT_NAMESPACE