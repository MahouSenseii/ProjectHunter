// Item/ItemInstance.cpp

#include "Item/ItemInstance.h"
#include "Item/Generation/AffixGenerator.h"
#include "AbilitySystemComponent.h"

UItemInstance::UItemInstance()
{
	UniqueID = FGuid::NewGuid();
	Seed = FMath::Rand();
}

// ═══════════════════════════════════════════════
// INITIALIZATION (Uses AffixGenerator)
// ═══════════════════════════════════════════════

void UItemInstance::Initialize(
	FDataTableRowHandle InBaseItemHandle,
	int32 InItemLevel,
	EItemRarity InRarity,
	bool bGenerateAffixes)
{
	// Call corruption version with no corruption
	InitializeWithCorruption(
		InBaseItemHandle,
		InItemLevel,
		InRarity,
		bGenerateAffixes,
		0.0f,   // No corruption chance
		false   // Don't force corrupted
	);
}

void UItemInstance::InitializeWithCorruption(
	FDataTableRowHandle InBaseItemHandle,
	int32 InItemLevel,
	EItemRarity InRarity,
	bool bGenerateAffixes,
	float CorruptionChance,
	bool bForceCorrupted)
{
	BaseItemHandle = InBaseItemHandle;
	ItemLevel = FMath::Clamp(InItemLevel, 1, 100);
	Rarity = InRarity;
	
	// Validate base data
	if (!HasValidBaseData())
	{
		UE_LOG(LogTemp, Error, TEXT("ItemInstance: Invalid base item handle: %s"),
			*InBaseItemHandle.RowName.ToString());
		return;
	}
	
	FItemBase* Base = GetBaseData();
	
	// Set rarity
	if (Rarity == EItemRarity::IR_None)
	{
		Rarity = Base->ItemRarity;
	}
	
	// Initialize by type
	EItemType Type = Base->ItemType;
	
	switch (Type)
	{
		case EItemType::IT_Weapon:
		case EItemType::IT_Armor:
		case EItemType::IT_Accessory:
		{
			// EQUIPMENT: Durability + Affixes
			Durability = FItemDurability();
			Durability.SetMaxDurability(Base->MaxDurability);
			
			// Generate affixes for Grade E and above (Grade F has no affixes)
			if (bGenerateAffixes && Rarity > EItemRarity::IR_GradeF)
			{
				FAffixGenerator Generator;
				
				// ═══════════════════════════════════════════════
				// CORRUPTION: Pass corruption params to generator
				// ═══════════════════════════════════════════════
				Stats = Generator.GenerateAffixes(
					*Base, 
					ItemLevel, 
					Rarity, 
					Seed,
					CorruptionChance,    // Per-affix corruption chance
					bForceCorrupted      // Force at least one corrupted
				);
				
				// Calculate corruption state from generated affixes
				CalculateCorruptionState();
			}
			else
			{
				// Copy implicits only
				Stats.Implicits = Base->ImplicitMods;
				for (FPHAttributeData& Implicit : Stats.Implicits)
				{
					Implicit.RollValue();
					Implicit.GenerateUID();
				}
			}
			
			bIdentified = !(Base->bCanBeIdentified);
			break;
		}
		
		case EItemType::IT_Consumable:
		{
			// CONSUMABLE: Uses + Cooldown
			Quantity = 1;
			RemainingUses = Base->ConsumableData.MaxUses > 0 ? Base->ConsumableData.MaxUses : 1;
			bIdentified = true;
			break;
		}
		
		case EItemType::IT_Material:
		case EItemType::IT_Currency:
		{
			// MATERIAL/CURRENCY: Stackable
			Quantity = 1;
			bIdentified = true;
			break;
		}
		
		case EItemType::IT_Quest:
		case EItemType::IT_Key:
		{
			// QUEST/KEY: Key item
			Quantity = 1;
			bIsKeyItem = true;
			bIsTradeable = false;
			bIsSoulbound = true;
			bIdentified = true;
			break;
		}
		
		default:
			Quantity = 1;
			bIdentified = true;
			break;
	}
	
	// Initialize economy
	bIsTradeable = Base->bIsTradeable;
	
	// Update weight
	UpdateTotalWeight();
	
	bCacheDirty = true;
}

// ═══════════════════════════════════════════════
// CORRUPTION SYSTEM
// ═══════════════════════════════════════════════

void UItemInstance::CalculateCorruptionState()
{
	bHasCorruptedAffixes = false;
	TotalCorruptionPoints = 0;
	
	// Check prefixes for negative rank points
	for (const FPHAttributeData& Affix : Stats.Prefixes)
	{
		int32 Points = Affix.GetRankPointValue();
		if (Points < 0)
		{
			bHasCorruptedAffixes = true;
			TotalCorruptionPoints += Points;
		}
	}
	
	// Check suffixes for negative rank points
	for (const FPHAttributeData& Affix : Stats.Suffixes)
	{
		int32 Points = Affix.GetRankPointValue();
		if (Points < 0)
		{
			bHasCorruptedAffixes = true;
			TotalCorruptionPoints += Points;
		}
	}
	
	// Check crafted mods
	for (const FPHAttributeData& Affix : Stats.Crafted)
	{
		int32 Points = Affix.GetRankPointValue();
		if (Points < 0)
		{
			bHasCorruptedAffixes = true;
			TotalCorruptionPoints += Points;
		}
	}
	
	// Corrupted items cannot be modified further
	if (bHasCorruptedAffixes)
	{
		bCanBeModified = false;
		
		UE_LOG(LogTemp, Log, TEXT("ItemInstance: Corruption detected! Points: %d"), TotalCorruptionPoints);
	}
}

TArray<FPHAttributeData> UItemInstance::GetCorruptedAffixes() const
{
	TArray<FPHAttributeData> Corrupted;
	
	// Check prefixes
	for (const FPHAttributeData& Affix : Stats.Prefixes)
	{
		if (Affix.IsCorruptedAffix())
		{
			Corrupted.Add(Affix);
		}
	}
	
	// Check suffixes
	for (const FPHAttributeData& Affix : Stats.Suffixes)
	{
		if (Affix.IsCorruptedAffix())
		{
			Corrupted.Add(Affix);
		}
	}
	
	// Check crafted
	for (const FPHAttributeData& Affix : Stats.Crafted)
	{
		if (Affix.IsCorruptedAffix())
		{
			Corrupted.Add(Affix);
		}
	}
	
	return Corrupted;
}

// ═══════════════════════════════════════════════
// NAME GENERATION (Hunter Manga Style)
// ═══════════════════════════════════════════════

FText UItemInstance::GetDisplayName()
{
	if (bHasNameBeenGenerated && !bCacheDirty)
	{
		return DisplayName;
	}
	
	FItemBase* Base = GetBaseData();
	if (!Base)
	{
		return FText::FromString("Unknown Item");
	}
	
	// Quest items and Grade SS (EX-Rank) use base name
	if (IsQuestItem() || Rarity == EItemRarity::IR_GradeSS || (IsEquipment() && Base->bIsUnique))
	{
		// EX-Rank items get special formatting
		if (Rarity == EItemRarity::IR_GradeSS)
		{
			DisplayName = FText::Format(
				FText::FromString("[{0}]"),
				Base->ItemName
			);
		}
		else
		{
			DisplayName = Base->ItemName;
		}
		
		bHasNameBeenGenerated = true;
		bCacheDirty = false;
		return DisplayName;
	}
	
	// Non-equipment uses base name
	if (!IsEquipment())
	{
		DisplayName = Base->ItemName;
		bHasNameBeenGenerated = true;
		bCacheDirty = false;
		return DisplayName;
	}
	
	// ═══════════════════════════════════════════════
	// CORRUPTION: Add "Corrupted" prefix for corrupted items
	// ═══════════════════════════════════════════════
	FString NamePrefix = "";
	if (bHasCorruptedAffixes)
	{
		NamePrefix = "Corrupted ";
	}
	
	// Equipment: Generate based on identification and rarity
	if (!bIdentified)
	{
		DisplayName = FText::Format(
			FText::FromString("Unidentified {0}{1}"),
			FText::FromString(NamePrefix),
			Base->ItemName
		);
	}
	else
	{
		switch (Rarity)
		{
			case EItemRarity::IR_GradeF:
			case EItemRarity::IR_GradeE:
				// Grade F/E: "Iron Sword" or "Corrupted Iron Sword"
				DisplayName = FText::Format(
					FText::FromString("{0}{1}"),
					FText::FromString(NamePrefix),
					Base->ItemName
				);
				break;
				
			case EItemRarity::IR_GradeD:
			case EItemRarity::IR_GradeC:
			case EItemRarity::IR_GradeB:
				// Grade D/C/B: "Flaming Iron Sword of Power"
				// TODO: Generate from affixes
				DisplayName = FText::Format(
					FText::FromString("{0}{1}"),
					FText::FromString(NamePrefix),
					Base->ItemName
				);
				break;
				
			case EItemRarity::IR_GradeA:
			case EItemRarity::IR_GradeS:
				// Grade A/S: "Demon-Slaying Blade"
				DisplayName = FText::Format(
					FText::FromString("{0}{1}"),
					FText::FromString(NamePrefix),
					GenerateRareName()
				);
				break;
				
			default:
				DisplayName = FText::Format(
					FText::FromString("{0}{1}"),
					FText::FromString(NamePrefix),
					Base->ItemName
				);
				break;
		}
	}
	
	bHasNameBeenGenerated = true;
	bCacheDirty = false;
	
	return DisplayName;
}

void UItemInstance::RegenerateDisplayName()
{
	bHasNameBeenGenerated = false;
	bCacheDirty = true;
	GetDisplayName();
}

FText UItemInstance::GenerateRareName() const
{
	// TODO: Implement procedural legendary name generation
	// Hunter Manga style names like:
	// - "Demon-Slaying Blade"
	// - "Dragon's Wrath"
	// - "Eternal Frost"
	
	FItemBase* Base = GetBaseData();
	return Base ? Base->ItemName : FText::FromString("Legendary Item");
}

// ═══════════════════════════════════════════════
// VISUAL GETTERS
// ═══════════════════════════════════════════════

UStaticMesh* UItemInstance::GetGroundMesh() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->StaticMesh.Get() : nullptr;
}

USkeletalMesh* UItemInstance::GetEquippedMesh() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->SkeletalMesh.Get() : nullptr;
}

UMaterialInstance* UItemInstance::GetInventoryIcon() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->ItemImage : nullptr;
}

FLinearColor UItemInstance::GetRarityColor() const
{
	// Corrupted items have a special color tint
	if (bHasCorruptedAffixes)
	{
		// Dark purple/red tint for corrupted items
		return FLinearColor(0.5f, 0.0f, 0.3f, 1.0f);
	}
	
	return GetItemRarityColor(Rarity);
}

// ═══════════════════════════════════════════════
// CONVENIENCE GETTERS
// ═══════════════════════════════════════════════

FText UItemInstance::GetBaseItemName() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->ItemName : FText::FromString("Unknown");
}

EItemType UItemInstance::GetItemType() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->ItemType : EItemType::IT_None;
}

EItemSubType UItemInstance::GetItemSubType() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->ItemSubType : EItemSubType::IST_None;
}

EEquipmentSlot UItemInstance::GetEquipmentSlot() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->EquipmentSlot : EEquipmentSlot::ES_None;
}

int32 UItemInstance::GetMaxStackSize() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->MaxStackSize : 1;
}

float UItemInstance::GetBaseWeight() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->BaseWeight : 0.0f;
}

bool UItemInstance::bIsTwoHanded() const
{
	FItemBase* Base = GetBaseData();
	return Base->WeaponHandle == EWeaponHandle::WH_TwoHanded;
}

// ═══════════════════════════════════════════════
// WEIGHT CALCULATION (Hunter Manga Style)
// ═══════════════════════════════════════════════

void UItemInstance::UpdateTotalWeight()
{
	float BaseWeight = GetBaseWeight();
	TotalWeight = BaseWeight * Quantity;
}

// ═══════════════════════════════════════════════
// AFFIX OPERATIONS (Equipment)
// ═══════════════════════════════════════════════

void UItemInstance::ApplyAffixesToCharacter(UAbilitySystemComponent* ASC)
{
	if (!ASC || !IsEquipment())
	{
		return;
	}
	
	RemoveAffixesFromCharacter(ASC);
	
	TArray<FPHAttributeData> AllAffixes = Stats.GetAllStats();
	
	for (const FPHAttributeData& Affix : AllAffixes)
	{
		// Skip unidentified affixes
		if (!Affix.bIsIdentified)
		{
			continue;
		}
		
		// Skip local weapon stats (applied to weapon directly, not character)
		if (Affix.bIsLocalToWeapon || Affix.bAffectsBaseWeaponStatsDirectly)
		{
			continue;
		}
		
		// Skip if no valid attribute
		if (!Affix.ModifiedAttribute.IsValid())
		{
			continue;
		}
		
		// ═══════════════════════════════════════════════
		// CORRUPTION: Corrupted affixes have NEGATIVE effects
		// They should still be applied - that's the point!
		// ═══════════════════════════════════════════════
		
		// TODO: Create and apply GameplayEffect for this affix
		
		UE_LOG(LogTemp, Log, TEXT("Applied affix: %s = %f (Corrupted: %s)"),
			*Affix.AttributeName.ToString(), 
			Affix.RolledStatValue,
			Affix.IsCorruptedAffix() ? TEXT("YES") : TEXT("NO"));
	}
	
	bEffectsActive = true;
}

void UItemInstance::RemoveAffixesFromCharacter(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}
	
	for (const FActiveGameplayEffectHandle& Handle : AppliedEffectHandles)
	{
		ASC->RemoveActiveGameplayEffect(Handle);
	}
	
	AppliedEffectHandles.Empty();
	bEffectsActive = false;
}

// ═══════════════════════════════════════════════
// CONSUMABLE OPERATIONS
// ═══════════════════════════════════════════════

bool UItemInstance::UseConsumable(AActor* Target)
{
	if (!CanUseConsumable() || !Target)
	{
		return false;
	}
	
	FItemBase* Base = GetBaseData();
	if (!Base)
	{
		return false;
	}
	
	// Apply effects
	if (!ApplyConsumableEffects(Target))
	{
		return false;
	}
	
	// Start cooldown
	CooldownRemaining = Base->ConsumableData.Cooldown;
	LastUseTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	
	// Reduce uses or quantity
	if (Base->ConsumableData.MaxUses > 1)
	{
		ReduceUses(1);
	}
	else
	{
		RemoveFromStack(1);
	}
	
	return true;
}

bool UItemInstance::CanUseConsumable() const
{
	if (!IsConsumable())
	{
		return false;
	}
	
	if (CooldownRemaining > 0.0f)
	{
		return false;
	}
	
	FItemBase* Base = GetBaseData();
	if (!Base)
	{
		return false;
	}
	
	if (Base->ConsumableData.MaxUses > 1 && RemainingUses <= 0)
	{
		return false;
	}
	
	if (Quantity <= 0)
	{
		return false;
	}
	
	return true;
}

float UItemInstance::GetCooldownProgress() const
{
	FItemBase* Base = GetBaseData();
	if (!Base || Base->ConsumableData.Cooldown <= 0.0f)
	{
		return 1.0f;
	}
	
	return FMath::Clamp(1.0f - (CooldownRemaining / Base->ConsumableData.Cooldown), 0.0f, 1.0f);
}

bool UItemInstance::ReduceUses(int32 Amount)
{
	RemainingUses = FMath::Max(0, RemainingUses - Amount);
	return RemainingUses <= 0;
}

bool UItemInstance::ApplyConsumableEffects(AActor* Target)
{
	// TODO: Apply GameplayEffects to target
	// Get target's ASC and apply Base->ConsumableEffects
	return true;
}

void UItemInstance::UpdateCooldown(float DeltaTime)
{
	if (CooldownRemaining > 0.0f)
	{
		CooldownRemaining = FMath::Max(0.0f, CooldownRemaining - DeltaTime);
	}
}

// ═══════════════════════════════════════════════
// IDENTIFICATION (Equipment)
// ═══════════════════════════════════════════════

void UItemInstance::Identify()
{
	if (!IsEquipment())
	{
		return;
	}
	
	bIdentified = true;
	
	for (FPHAttributeData& Affix : Stats.Prefixes)
	{
		Affix.bIsIdentified = true;
	}
	
	for (FPHAttributeData& Affix : Stats.Suffixes)
	{
		Affix.bIsIdentified = true;
	}
	
	for (FPHAttributeData& Affix : Stats.Implicits)
	{
		Affix.bIsIdentified = true;
	}
	
	for (FPHAttributeData& Affix : Stats.Crafted)
	{
		Affix.bIsIdentified = true;
	}
	
	RegenerateDisplayName();
}

bool UItemInstance::HasUnidentifiedAffixes() const
{
	return IsEquipment() && Stats.HasUnidentifiedStats();
}

// ═══════════════════════════════════════════════
// ITEM TYPE CHECKS
// ═══════════════════════════════════════════════

bool UItemInstance::IsEquipment() const
{
	EItemType Type = GetItemType();
	return Type == EItemType::IT_Weapon 
	    || Type == EItemType::IT_Armor 
	    || Type == EItemType::IT_Accessory;
}

bool UItemInstance::IsConsumable() const
{
	return GetItemType() == EItemType::IT_Consumable;
}

bool UItemInstance::IsMaterial() const
{
	return GetItemType() == EItemType::IT_Material;
}

bool UItemInstance::IsQuestItem() const
{
	return GetItemType() == EItemType::IT_Quest || bIsKeyItem;
}

bool UItemInstance::IsCurrency() const
{
	return GetItemType() == EItemType::IT_Currency;
}

bool UItemInstance::IsKeyItem() const
{
	return GetItemType() == EItemType::IT_Key || bIsKeyItem;
}

// ═══════════════════════════════════════════════
// ITEM STATE CHECKS
// ═══════════════════════════════════════════════

bool UItemInstance::CanBeEquipped() const
{
	return IsEquipment() && !IsBroken();
}

bool UItemInstance::IsStackable() const
{
	FItemBase* Base = GetBaseData();
	return Base ? Base->bStackable : false;
}

bool UItemInstance::CanStackWith(const UItemInstance* Other) const
{
	if (!Other || !IsStackable())
	{
		return false;
	}
	
	if (BaseItemHandle != Other->BaseItemHandle)
	{
		return false;
	}
	
	// Equipment with affixes cannot stack
	if (IsEquipment() && Stats.bAffixesGenerated)
	{
		return false;
	}
	
	// Consumables with different uses cannot stack
	if (IsConsumable() && RemainingUses != Other->RemainingUses)
	{
		return false;
	}
	
	return true;
}

bool UItemInstance::IsConsumed() const
{
	if (IsConsumable())
	{
		return RemainingUses <= 0 || Quantity <= 0;
	}
	
	return Quantity <= 0;
}

// ═══════════════════════════════════════════════
// STACKING
// ═══════════════════════════════════════════════

int32 UItemInstance::AddToStack(int32 Amount)
{
	if (!IsStackable() || Amount <= 0)
	{
		return Amount;
	}
	
	int32 MaxStack = GetMaxStackSize();
	int32 Available = MaxStack - Quantity;
	int32 ToAdd = FMath::Min(Amount, Available);
	
	Quantity += ToAdd;
	UpdateTotalWeight();
	
	return Amount - ToAdd;
}

int32 UItemInstance::RemoveFromStack(int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}
	
	int32 ToRemove = FMath::Min(Amount, Quantity);
	Quantity -= ToRemove;
	UpdateTotalWeight();
	
	return ToRemove;
}

UItemInstance* UItemInstance::SplitStack(int32 Amount)
{
	if (!IsStackable() || Amount <= 0 || Amount >= Quantity)
	{
		return nullptr;
	}
	
	UItemInstance* NewInstance = NewObject<UItemInstance>(GetTransientPackage(), GetClass());
	
	NewInstance->BaseItemHandle = BaseItemHandle;
	NewInstance->ItemLevel = ItemLevel;
	NewInstance->Rarity = Rarity;
	NewInstance->Quantity = Amount;
	NewInstance->RemainingUses = RemainingUses;
	NewInstance->UniqueID = FGuid::NewGuid();
	
	// Copy corruption state
	NewInstance->bHasCorruptedAffixes = bHasCorruptedAffixes;
	NewInstance->TotalCorruptionPoints = TotalCorruptionPoints;
	NewInstance->bCanBeModified = bCanBeModified;
	
	RemoveFromStack(Amount);
	NewInstance->UpdateTotalWeight();
	
	return NewInstance;
}

int32 UItemInstance::GetRemainingStackSpace() const
{
	if (!IsStackable())
	{
		return 0;
	}
	
	return FMath::Max(0, GetMaxStackSize() - Quantity);
}

// ═══════════════════════════════════════════════
// VALUE & ECONOMY (Hunter Manga Style)
// ═══════════════════════════════════════════════

int32 UItemInstance::GetCalculatedValue() const
{
	FItemBase* Base = GetBaseData();
	if (!Base)
	{
		return 0;
	}
	
	float Value = Base->Value;
	
	// Multiply by quantity for stackables
	if (IsStackable())
	{
		Value *= Quantity;
	}
	
	// Add value from affixes (equipment only)
	if (IsEquipment())
	{
		Value += Stats.GetTotalAffixValue() * 10.0f;
		
		// Hunter Manga: Exponential value increase by grade
		switch (Rarity)
		{
			case EItemRarity::IR_GradeF: Value *= 1.0f; break;
			case EItemRarity::IR_GradeE: Value *= 1.5f; break;
			case EItemRarity::IR_GradeD: Value *= 2.5f; break;
			case EItemRarity::IR_GradeC: Value *= 5.0f; break;
			case EItemRarity::IR_GradeB: Value *= 10.0f; break;
			case EItemRarity::IR_GradeA: Value *= 25.0f; break;
			case EItemRarity::IR_GradeS: Value *= 100.0f; break;
			case EItemRarity::IR_GradeSS: Value *= 1000.0f; break;  // EX-Rank!
			default: break;
		}
		
		// ═══════════════════════════════════════════════
		// CORRUPTION: Corrupted items worth LESS
		// ═══════════════════════════════════════════════
		if (bHasCorruptedAffixes)
		{
			// Reduce value based on corruption severity
			float CorruptionPenalty = FMath::Clamp(
				FMath::Abs(TotalCorruptionPoints) * 0.05f, 
				0.0f, 
				0.5f
			);
			Value *= (1.0f - CorruptionPenalty);
		}
		
		// Broken items worth 10%
		if (IsBroken())
		{
			Value *= 0.1f;
		}
	}
	
	// Apply value modifier
	Value *= (1.0f + ValueModifier);
	
	// Reduce value for partially used consumables
	if (IsConsumable())
	{
		FItemBase* BaseData = GetBaseData();
		if (BaseData && BaseData->ConsumableData.MaxUses > 1)
		{
			Value *= (float)RemainingUses / (float)BaseData->ConsumableData.MaxUses;
		}
	}
	
	return FMath::Max(0, FMath::RoundToInt(Value));
}

int32 UItemInstance::GetSellValue(float SellPercentage) const
{
	return FMath::RoundToInt(GetCalculatedValue() * FMath::Clamp(SellPercentage, 0.0f, 1.0f));
}

// ═══════════════════════════════════════════════
// BASE DATA ACCESS (Cached)
// ═══════════════════════════════════════════════

FItemBase* UItemInstance::GetBaseData() const
{
	if (!bCacheDirty && CachedBaseData)
	{
		return CachedBaseData;
	}
	
	if (!BaseItemHandle.IsNull())
	{
		CachedBaseData = BaseItemHandle.GetRow<FItemBase>("GetBaseData");
		bCacheDirty = false;
		return CachedBaseData;
	}
	
	return nullptr;
}

bool UItemInstance::GetBaseDataBP(FItemBase& OutBaseData) const
{
	FItemBase* Base = GetBaseData();
	if (Base)
	{
		OutBaseData = *Base;
		return true;
	}
	return false;
}

bool UItemInstance::HasValidBaseData() const
{
	return GetBaseData() != nullptr;
}

void UItemInstance::InvalidateBaseCache()
{
	bCacheDirty = true;
	CachedBaseData = nullptr;
}

// ═══════════════════════════════════════════════
// SERIALIZATION HELPERS
// ═══════════════════════════════════════════════

void UItemInstance::PrepareForSave()
{
	AppliedEffectHandles.Empty();
	bEffectsActive = false;
	InvalidateBaseCache();
}

void UItemInstance::PostLoadInitialize()
{
	bCacheDirty = true;
	InvalidateBaseCache();
	
	if (!HasValidBaseData())
	{
		UE_LOG(LogTemp, Error, TEXT("ItemInstance: Base data no longer exists for %s!"),
			*UniqueID.ToString());
	}
	
	// Recalculate corruption state after load
	CalculateCorruptionState();
}