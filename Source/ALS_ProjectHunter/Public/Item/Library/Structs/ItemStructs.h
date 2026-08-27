// Item/Library/Structs/ItemStructs.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Equipment/Actors/EquippedItemRuntimeActor.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "Item/Library/Structs/ItemAttachmentStructs.h"
#include "Item/Library/Structs/ItemBaseStatStructs.h"
#include "Item/Library/Structs/ItemConsumableStructs.h"
#include "Item/Library/Structs/ItemDurabilityStructs.h"
#include "Item/Library/Structs/ItemRequirementStructs.h"
#include "Item/Library/Structs/ItemRuneStructs.h"
#include "Item/Library/Structs/ItemStatsStructs.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "ItemStructs.generated.h"

class AActor;
class UStaticMesh;
class USkeletalMesh;
class UTexture2D;


/**
 * Base item definition (DataTable row)
 * Contains all static/shared data for an item type
 */
USTRUCT(BlueprintType)
struct FItemBase : public FTableRowBase
{
	GENERATED_BODY()


	/** Is this a unique item? (If false, name is auto-generated from affixes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Identity")
	bool bIsUnique = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Identity")
	FName ItemID = NAME_None;

	/**
	 * Base display name for the item ("Iron Sword", "Leather Hood").
	 * Every item needs one - non-unique gear composes its final name around it
	 * ("Flaming Iron Sword of Haste"), uniques display it directly/bracketed.
	 * (Previously hidden behind bIsUnique, which left all non-unique items
	 * with blank names in-game.)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Display")
	FText ItemName = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Display", meta = (MultiLine = "true"))
	FText ItemDescription = FText::GetEmpty();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Class")
	EItemType ItemType = EItemType::IT_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Class")
	EItemSubType ItemSubType = EItemSubType::IST_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Class")
	EALSOverlayState OverlayState = EALSOverlayState::Default;

	/** Default affix-roll budget when Initialize receives IR_None; runtime grade is score-derived. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Class")
	EItemRarity ItemRarity = EItemRarity::IR_GradeF;

	/** Starting item-power score before implicits and rolled affixes are added. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Power", meta = (ClampMin = "0.0"))
	float BasePowerValue = 10.0f;

	/** Only show for equipment types */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Class",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	EEquipmentSlot EquipmentSlot = EEquipmentSlot::ES_None;

	/** Only show for weapons */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Class",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon", EditConditionHides))
	EWeaponHandle WeaponHandle = EWeaponHandle::WH_None;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Visual")
	TObjectPtr<UStaticMesh> StaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Visual")
	TObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;

	/** Inventory / menu icon. Texture, not a material - the slots draw it directly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Visual")
	TObjectPtr<UTexture2D> ItemImage = nullptr;

	/** Spawn a runtime actor for active/special equipment instead of using only a mesh representation. Weapons always use a runtime actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Visual",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	bool bUseRuntimeActor = false;

	/** Runtime actor class used for active/special equipment behavior. Prefer a Blueprint child of AEquippedItemRuntimeActor. Weapons fall back to AEquippedItemRuntimeActor if unset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Visual",
		meta = (EditCondition = "bUseRuntimeActor && (ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory)", EditConditionHides))
	TSubclassOf<AEquippedItemRuntimeActor> RuntimeActorClass = nullptr;

	/** Legacy compatibility for older weapon rows. Prefer bUseRuntimeActor. */
	UPROPERTY()
	bool bUseWeaponActor = false;

	/** Legacy compatibility for older weapon rows. Prefer RuntimeActorClass. */
	UPROPERTY()
	TSubclassOf<AActor> WeaponActorClass = nullptr;


	/**
	 * Flip the mesh 180 on the Pitch axis when displayed on the ground.
	 * Enable for items (e.g. swords) whose static mesh naturally points upward
	 * when placed at ZeroRotator, so they rest blade-down correctly.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|GroundDisplay")
	bool bFlipGroundMeshRotation = false;

	/**
	 * Additional rotation offset applied when the item is placed on the ground,
	 * on top of any flip. Use to fine-tune the resting orientation per-item.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|GroundDisplay",
		meta = (EditCondition = "!bFlipGroundMeshRotation || true"))
	FRotator GroundMeshRotationOffset = FRotator::ZeroRotator;


	/** Base weight for single item  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Weight", meta = (ClampMin = "0.0"))
	float BaseWeight = 0.1f;

	/**
	 * Can this item stack?
	 * NOTE: Weapons, Armor, Accessories are NEVER stackable (unique instances)
	 * Only Consumables, Materials, Currency can stack
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Stacking",
		meta = (EditCondition = "ItemType != EItemType::IT_Weapon && ItemType != EItemType::IT_Armor && ItemType != EItemType::IT_Accessory", EditConditionHides))
	bool bStackable = false;

	/** Maximum stack size (only for stackable items) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Stacking",
		meta = (EditCondition = "bStackable && ItemType != EItemType::IT_Weapon && ItemType != EItemType::IT_Armor && ItemType != EItemType::IT_Accessory", EditConditionHides, ClampMin = "1"))
	int32 MaxStackSize = 1;

	/** Scale weight with quantity? (only relevant for stackables) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Weight",
		meta = (EditCondition = "bStackable", EditConditionHides))
	bool bScaleWeightWithQuantity = true;


	/** Base gold value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Economy", meta = (ClampMin = "0"))
	int32 Value = 0;

	/** Value modifier percentage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Economy")
	float ValueModifier = 0.0f;

	/** Can be traded with other hunters? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Economy")
	bool bIsTradeable = true;


	/** Does this item need to be identified? (Only equipment) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Flags",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	bool bCanBeIdentified = true;

	/** Force every generated affix to start identified even when the item type normally supports identification. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Flags",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	bool bForceAllAffixesIdentified = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Attachment",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	FName AttachmentSocket = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Attachment",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	TMap<FName, FName> ContextualSockets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Attachment",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	FItemAttachmentRules AttachmentRules;

	/**
	 * Relative transform applied after the item is snapped to its hand socket.
	 * Use this to fine-tune position, rotation, and scale per item without
	 * modifying the skeleton socket itself.
	 * Location = offset from socket origin (cm).
	 * Rotation = relative rotation from socket orientation.
	 * Scale    = mesh scale multiplier (1,1,1 = no change).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Attachment",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	FTransform HandAttachTransform = FTransform::Identity;



	/** Only show for weapons */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Stats",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon", EditConditionHides))
	FBaseWeaponStats WeaponStats;

	/** Only show for armor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Stats",
		meta = (EditCondition = "ItemType == EItemType::IT_Armor", EditConditionHides))
	FBaseArmorStats ArmorStats;

	/** Only show for equipment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Stats",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	FItemStatRequirement StatRequirements;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Durability",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	float MaxDurability = 100.0f;


	/** Always-present affixes (e.g., "+10 Fire Resistance" on Fire Cloak) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Affixes",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	TArray<FPHAttributeData> ImplicitMods;

	/** Prefix definitions this base item can roll. Uses the generator's shared prefix table when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Affixes",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	TObjectPtr<UDataTable> PrefixAffixTable = nullptr;

	/** Suffix definitions this base item can roll. Uses the generator's shared suffix table when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Affixes",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	TObjectPtr<UDataTable> SuffixAffixTable = nullptr;

	/** Enchant definitions allowed on this base item. Uses the generator's shared enchant table when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Affixes",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	TObjectPtr<UDataTable> EnchantAffixTable = nullptr;


	/** Fixed affixes for unique items (not randomly generated) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Unique",
		meta = (EditCondition = "bIsUnique", EditConditionHides))
	TArray<FPHAttributeData> UniqueAffixes;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Consumable",
		meta = (EditCondition = "ItemType == EItemType::IT_Consumable", EditConditionHides))
	FConsumableData ConsumableData;

	int32 GetMaxUses() const;
	float GetCooldown() const;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Runes",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	int32 MaxRuneSockets = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Runes",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	int32 MaxEnhancementLevel = 15;

	bool IsValid() const;

	bool IsValidForInventory() const;

	bool IsWeapon() const;
	bool IsArmor() const;
	bool IsAccessory() const;
	bool IsEquippable() const;
	bool IsConsumable() const;
	bool IsMaterial() const;
	bool IsCurrency() const;

	bool UsesRuntimeActor() const;
	TSubclassOf<AActor> GetRuntimeActorClass() const;

	/**
	 * Returns only an explicitly mapped context socket.
	 * Callers use AttachmentSocket as fallback.
	 */
	FName GetSocketForContext(FName Context) const;

	float GetCalculatedValue(int32 Quantity = 1, EItemRarity InstanceRarity = EItemRarity::IR_None) const;
	float GetTotalWeight(int32 Quantity = 1) const;

	FItemBase() = default;
};
