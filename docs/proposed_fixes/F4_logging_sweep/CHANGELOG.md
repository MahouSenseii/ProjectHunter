# F4 Logging Sweep Changelog

## Summary
This changelog documents all raw `UE_LOG(..., Warning|Error, ...)` calls found in `/Private/Systems/` and `/Public/Systems/` that need conversion to the `PH_LOG_WARNING` and `PH_LOG_ERROR` macros from `Public/Core/Logging/ProjectHunterLogMacros.h`.

Macro signature:
```cpp
PH_LOG_WARNING(Category, "Format string", ...)
PH_LOG_ERROR(Category, "Format string", ...)
```

Note: The format string should be a plain `"..."` string literal (not wrapped in `TEXT(...)`).

---

## Private/Systems/Combat/Components/DoTManager.cpp

### LINE 222
**BEFORE:**
```cpp
UE_LOG(LogDoTManager, Warning,
    TEXT("ApplyDoTEffect: Target '%s' has no ASC"), *Target->GetName());
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogDoTManager, "ApplyDoTEffect: Target '%s' has no ASC", *Target->GetName());
```

### LINE 230
**BEFORE:**
```cpp
UE_LOG(LogDoTManager, Warning,
    TEXT("ApplyDoTEffect: Owner '%s' has no ASC"), *GetOwner()->GetName());
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogDoTManager, "ApplyDoTEffect: Owner '%s' has no ASC", *GetOwner()->GetName());
```

---

## Private/Systems/Equipment/Components/EquipmentManager.cpp

### LINE 87
**BEFORE:**
```cpp
UE_LOG(LogEquipmentManager, Error,
    TEXT("EquipmentManager: No InventoryManager found on '%s'."), *GetNameSafe(Owner));
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogEquipmentManager, "EquipmentManager: No InventoryManager found on '%s'.", *GetNameSafe(Owner));
```

### LINE 105
**BEFORE:**
```cpp
UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager::EquipItem: Null item."));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogEquipmentManager, "EquipmentManager::EquipItem: Null item.");
```

### LINE 140
**BEFORE:**
```cpp
UE_LOG(LogEquipmentManager, Warning,
    TEXT("EquipmentManager: Failed to return unequipped item to inventory."));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogEquipmentManager, "EquipmentManager: Failed to return unequipped item to inventory.");
```

### LINE 181
**BEFORE:**
```cpp
UE_LOG(LogEquipmentManager, Warning,
    TEXT("EquipmentManager::UnequipAll: Must be called on server."));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogEquipmentManager, "EquipmentManager::UnequipAll: Must be called on server.");
```

### LINE 241
**BEFORE:**
```cpp
UE_LOG(LogEquipmentManager, Warning,
    TEXT("EquipmentManager::TryEquipGroundPickupItem: Must be called on the server."));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogEquipmentManager, "EquipmentManager::TryEquipGroundPickupItem: Must be called on the server.");
```

### LINE 330
**BEFORE:**
```cpp
UE_LOG(LogEquipmentManager, Warning, TEXT("EquipmentManager: Item has no base data."));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogEquipmentManager, "EquipmentManager: Item has no base data.");
```

### LINE 336
**BEFORE:**
```cpp
UE_LOG(LogEquipmentManager, Warning,
    TEXT("EquipmentManager: Item '%s' is not equippable."), *GetNameSafe(Item));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogEquipmentManager, "EquipmentManager: Item '%s' is not equippable.", *GetNameSafe(Item));
```

### LINE 348
**BEFORE:**
```cpp
UE_LOG(LogEquipmentManager, Warning,
    TEXT("EquipmentManager: Could not determine slot for item '%s'."), *GetNameSafe(Item));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogEquipmentManager, "EquipmentManager: Could not determine slot for item '%s'.", *GetNameSafe(Item));
```

### LINE 358
**BEFORE:**
```cpp
UE_LOG(LogEquipmentManager, Warning,
    TEXT("EquipmentManager: Item '%s' cannot be equipped to slot %d."),
    *GetNameSafe(Item), static_cast<int32>(Slot));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogEquipmentManager, "EquipmentManager: Item '%s' cannot be equipped to slot %d.",
    *GetNameSafe(Item), static_cast<int32>(Slot));
```

### LINE 390
**BEFORE:**
```cpp
UE_LOG(LogEquipmentManager, Warning,
    TEXT("EquipmentManager: Failed to return displaced two-hand item to inventory."));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogEquipmentManager, "EquipmentManager: Failed to return displaced two-hand item to inventory.");
```

### LINE 410
**BEFORE:**
```cpp
UE_LOG(LogEquipmentManager, Warning,
    TEXT("EquipmentManager: Failed to return displaced item to inventory."));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogEquipmentManager, "EquipmentManager: Failed to return displaced item to inventory.");
```

---

## Private/Systems/Equipment/Components/EquipmentPresentationComponent.cpp

### LINE 129
**BEFORE:**
```cpp
UE_LOG(LogEquipmentPresentation, Warning,
    TEXT("UEquipmentPresentationComponent::AttachItemVisual: No CharacterMesh on '%s'. Visual skipped."),
    *GetNameSafe(GetOwner()));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogEquipmentPresentation, "UEquipmentPresentationComponent::AttachItemVisual: No CharacterMesh on '%s'. Visual skipped.",
    *GetNameSafe(GetOwner()));
```

### LINE 138
**BEFORE:**
```cpp
UE_LOG(LogEquipmentPresentation, Warning,
    TEXT("UEquipmentPresentationComponent::AttachItemVisual: Item '%s' has no base data."),
    *GetNameSafe(Item));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogEquipmentPresentation, "UEquipmentPresentationComponent::AttachItemVisual: Item '%s' has no base data.",
    *GetNameSafe(Item));
```

### LINE 147
**BEFORE:**
```cpp
UE_LOG(LogEquipmentPresentation, Warning,
    TEXT("UEquipmentPresentationComponent::AttachItemVisual: Socket '%s' does not exist on '%s'. Visual skipped."),
    *SocketName.ToString(), *GetNameSafe(GetOwner()));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogEquipmentPresentation, "UEquipmentPresentationComponent::AttachItemVisual: Socket '%s' does not exist on '%s'. Visual skipped.",
    *SocketName.ToString(), *GetNameSafe(GetOwner()));
```

### LINE 166
**BEFORE:**
```cpp
UE_LOG(LogEquipmentPresentation, Warning,
    TEXT("UEquipmentPresentationComponent: Item '%s' requests a runtime actor but RuntimeActorClass is null. Falling back to mesh."),
    *GetNameSafe(Item));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogEquipmentPresentation, "UEquipmentPresentationComponent: Item '%s' requests a runtime actor but RuntimeActorClass is null. Falling back to mesh.",
    *GetNameSafe(Item));
```

### LINE 214
**BEFORE:**
```cpp
UE_LOG(LogEquipmentPresentation, Warning,
    TEXT("UEquipmentPresentationComponent::SpawnWeaponActor: Weapon '%s' has no valid runtime actor class. Defaulting to AEquippedItemRuntimeActor."),
    *GetNameSafe(Item));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogEquipmentPresentation, "UEquipmentPresentationComponent::SpawnWeaponActor: Weapon '%s' has no valid runtime actor class. Defaulting to AEquippedItemRuntimeActor.",
    *GetNameSafe(Item));
```

### LINE 241
**BEFORE:**
```cpp
UE_LOG(LogEquipmentPresentation, Error,
    TEXT("UEquipmentPresentationComponent::SpawnWeaponActor: Failed to spawn actor of class '%s'."),
    *GetNameSafe(RuntimeActorClass));
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogEquipmentPresentation, "UEquipmentPresentationComponent::SpawnWeaponActor: Failed to spawn actor of class '%s'.",
    *GetNameSafe(RuntimeActorClass));
```

### LINE 309
**BEFORE:**
```cpp
UE_LOG(LogEquipmentPresentation, Warning,
    TEXT("UEquipmentPresentationComponent::SpawnWeaponMesh: Item '%s' has no mesh asset."),
    *GetNameSafe(Item));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogEquipmentPresentation, "UEquipmentPresentationComponent::SpawnWeaponMesh: Item '%s' has no mesh asset.",
    *GetNameSafe(Item));
```

---

## Private/Systems/Progression/Components/CharacterProgressionManager.cpp

### LINE 86
**BEFORE:**
```cpp
UE_LOG(LogCharacterProgressionManager, Warning, TEXT("AwardExperienceFromKill: KilledCharacter is null"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogCharacterProgressionManager, "AwardExperienceFromKill: KilledCharacter is null");
```

### LINE 92
**BEFORE:**
```cpp
UE_LOG(LogCharacterProgressionManager, Warning, TEXT("AwardExperienceFromKill: Called on client"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogCharacterProgressionManager, "AwardExperienceFromKill: Called on client");
```

### LINE 103
**BEFORE:**
```cpp
UE_LOG(LogCharacterProgressionManager, Warning, TEXT("AwardExperienceFromKill: No AttributeSet found"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogCharacterProgressionManager, "AwardExperienceFromKill: No AttributeSet found");
```

### LINE 231
**BEFORE:**
```cpp
UE_LOG(LogCharacterProgressionManager, Warning, TEXT("LevelUp: Already at max level (%d)"), MaxLevel);
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogCharacterProgressionManager, "LevelUp: Already at max level (%d)", MaxLevel);
```

### LINE 311
**BEFORE:**
```cpp
UE_LOG(LogCharacterProgressionManager, Warning, TEXT("SpendStatPoint: Called on client"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogCharacterProgressionManager, "SpendStatPoint: Called on client");
```

### LINE 318
**BEFORE:**
```cpp
UE_LOG(LogCharacterProgressionManager, Warning, TEXT("SpendStatPoint: No unspent stat points"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogCharacterProgressionManager, "SpendStatPoint: No unspent stat points");
```

### LINE 325
**BEFORE:**
```cpp
UE_LOG(LogCharacterProgressionManager, Warning, TEXT("SpendStatPoint: Invalid attribute name"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogCharacterProgressionManager, "SpendStatPoint: Invalid attribute name");
```

### LINE 460
**BEFORE:**
```cpp
UE_LOG(LogCharacterProgressionManager, Error, TEXT("ApplyStatPointToAttribute: ASC is null"));
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogCharacterProgressionManager, "ApplyStatPointToAttribute: ASC is null");
```

### LINE 468
**BEFORE:**
```cpp
UE_LOG(LogCharacterProgressionManager, Error,
    TEXT("ApplyStatPointToAttribute: Unknown primary attribute '%s'"), *AttributeName.ToString());
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogCharacterProgressionManager, "ApplyStatPointToAttribute: Unknown primary attribute '%s'",
    *AttributeName.ToString());
```

### LINE 477
**BEFORE:**
```cpp
UE_LOG(LogCharacterProgressionManager, Warning,
    TEXT("ApplyStatPointToAttribute: No GE class configured for attribute '%s'. "
         "Add an entry to StatPointGEClasses in the Blueprint defaults."),
    *AttributeName.ToString());
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogCharacterProgressionManager, "ApplyStatPointToAttribute: No GE class configured for attribute '%s'. Add an entry to StatPointGEClasses in the Blueprint defaults.",
    *AttributeName.ToString());
```

**NOTE:** Multi-line format string concatenated into single literal.

### LINE 501
**BEFORE:**
```cpp
UE_LOG(LogCharacterProgressionManager, Error,
    TEXT("ApplyStatPointToAttribute: GE application failed for '%s'"), *AttributeName.ToString());
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogCharacterProgressionManager, "ApplyStatPointToAttribute: GE application failed for '%s'",
    *AttributeName.ToString());
```

### LINE 521
**BEFORE:**
```cpp
UE_LOG(LogCharacterProgressionManager, Warning,
    TEXT("RemoveStatPointFromAttribute: No tracked GE handles for '%s'. "
         "Attribute will not be adjusted."), *AttributeName.ToString());
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogCharacterProgressionManager, "RemoveStatPointFromAttribute: No tracked GE handles for '%s'. Attribute will not be adjusted.",
    *AttributeName.ToString());
```

**NOTE:** Multi-line format string concatenated into single literal.

---

## Private/Systems/Stats/Components/StatsManager.cpp

### LINE 136
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("%s"), *Message);
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "%s", *Message);
```

### LINE 311
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyEquipmentStats: Invalid item"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "StatsManager::ApplyEquipmentStats: Invalid item");
```

### LINE 318
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Error, TEXT("StatsManager::ApplyEquipmentStats: No AbilitySystemComponent"));
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogStatsManager, "StatsManager::ApplyEquipmentStats: No AbilitySystemComponent");
```

### LINE 324
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyEquipmentStats: Must be called on server"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "StatsManager::ApplyEquipmentStats: Must be called on server");
```

### LINE 331
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("StatsManager: Equipment stats already applied for %s"), *Item->GetName());
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "StatsManager: Equipment stats already applied for %s", *Item->GetName());
```

### LINE 348
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Error, TEXT("StatsManager: Failed to create equipment effect for %s"), *Item->GetName());
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogStatsManager, "StatsManager: Failed to create equipment effect for %s", *Item->GetName());
```

### LINE 365
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Error, TEXT("StatsManager: Failed to apply equipment effect for %s"), *Item->GetName());
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogStatsManager, "StatsManager: Failed to apply equipment effect for %s", *Item->GetName());
```

### LINE 373
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::RemoveEquipmentStats: Invalid item"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "StatsManager::RemoveEquipmentStats: Invalid item");
```

### LINE 380
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Error, TEXT("StatsManager::RemoveEquipmentStats: No AbilitySystemComponent"));
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogStatsManager, "StatsManager::RemoveEquipmentStats: No AbilitySystemComponent");
```

### LINE 386
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::RemoveEquipmentStats: Must be called on server"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "StatsManager::RemoveEquipmentStats: Must be called on server");
```

### LINE 394
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("StatsManager: No active equipment effect found for %s"), *Item->GetName());
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "StatsManager: No active equipment effect found for %s", *Item->GetName());
```

### LINE 493
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Error, TEXT("StatsManager::ApplyGameplayEffectToSelf: No AbilitySystemComponent"));
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogStatsManager, "StatsManager::ApplyGameplayEffectToSelf: No AbilitySystemComponent");
```

### LINE 499
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyGameplayEffectToSelf: Invalid effect class"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "StatsManager::ApplyGameplayEffectToSelf: Invalid effect class");
```

### LINE 505
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyGameplayEffectToSelf: Must be called on server"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "StatsManager::ApplyGameplayEffectToSelf: Must be called on server");
```

### LINE 515
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Error, TEXT("StatsManager::ApplyGameplayEffectToSelf: Failed to create spec"));
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogStatsManager, "StatsManager::ApplyGameplayEffectToSelf: Failed to create spec");
```

### LINE 528
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Error, TEXT("StatsManager::ApplyGameplayEffectToTarget: No source AbilitySystemComponent"));
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogStatsManager, "StatsManager::ApplyGameplayEffectToTarget: No source AbilitySystemComponent");
```

### LINE 534
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyGameplayEffectToTarget: Invalid target actor"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "StatsManager::ApplyGameplayEffectToTarget: Invalid target actor");
```

### LINE 540
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyGameplayEffectToTarget: Invalid effect class"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "StatsManager::ApplyGameplayEffectToTarget: Invalid effect class");
```

### LINE 546
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyGameplayEffectToTarget: Must be called on server"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "StatsManager::ApplyGameplayEffectToTarget: Must be called on server");
```

### LINE 553
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyGameplayEffectToTarget: Target %s does not implement AbilitySystemInterface"),
    *TargetActor->GetName());
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "StatsManager::ApplyGameplayEffectToTarget: Target %s does not implement AbilitySystemInterface",
    *TargetActor->GetName());
```

### LINE 561
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("StatsManager::ApplyGameplayEffectToTarget: Target %s has no AbilitySystemComponent"),
    *TargetActor->GetName());
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "StatsManager::ApplyGameplayEffectToTarget: Target %s has no AbilitySystemComponent",
    *TargetActor->GetName());
```

### LINE 572
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Error, TEXT("StatsManager::ApplyGameplayEffectToTarget: Failed to create spec"));
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogStatsManager, "StatsManager::ApplyGameplayEffectToTarget: Failed to create spec");
```

### LINE 705
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("StatsManager: Invalid attribute for stat '%s'"), 
    *Stat.AttributeName.ToString());
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "StatsManager: Invalid attribute for stat '%s'",
    *Stat.AttributeName.ToString());
```

### LINE 718
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("StatsManager: No valid modifiers for item %s"), *Item->GetName());
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "StatsManager: No valid modifiers for item %s", *Item->GetName());
```

### LINE 771
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("StatsManager: Unsupported ModifyType %d for attribute %s"), 
    static_cast<int32>(Stat.ModifyType), *Attribute.GetName());
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "StatsManager: Unsupported ModifyType %d for attribute %s",
    static_cast<int32>(Stat.ModifyType), *Attribute.GetName());
```

### LINE 855
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Error, TEXT("SetNumericAttributeByName: No AbilitySystemComponent found"));
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogStatsManager, "SetNumericAttributeByName: No AbilitySystemComponent found");
```

### LINE 882
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("SetNumericAttributeByName: Invalid resolved attribute '%s'"), *AttributeName.ToString());
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "SetNumericAttributeByName: Invalid resolved attribute '%s'", *AttributeName.ToString());
```

### LINE 1335
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("InitializeFromDataAsset: StatsData is null"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "InitializeFromDataAsset: StatsData is null");
```

### LINE 1342
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Error, TEXT("InitializeFromDataAsset: Owner is null"));
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogStatsManager, "InitializeFromDataAsset: Owner is null");
```

### LINE 1355
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Error, TEXT("InitializeFromDataAsset: No AbilitySystemComponent found"));
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogStatsManager, "InitializeFromDataAsset: No AbilitySystemComponent found");
```

### LINE 1361
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("InitializeFromDataAsset: Must be called on server"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "InitializeFromDataAsset: Must be called on server");
```

### LINE 1368
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Error, TEXT("InitializeFromDataAsset: No valid SourceAttributeSetClass for asset=%s"),
    *GetNameSafe(StatsData));
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogStatsManager, "InitializeFromDataAsset: No valid SourceAttributeSetClass for asset=%s",
    *GetNameSafe(StatsData));
```

### LINE 1377
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Error,
    TEXT("InitializeFromDataAsset: Failed to create AttributeSet instance of class '%s' for owner '%s'"),
    *GetNameSafe(SourceAttributeSetClass), *GetNameSafe(Owner));
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogStatsManager, "InitializeFromDataAsset: Failed to create AttributeSet instance of class '%s' for owner '%s'",
    *GetNameSafe(SourceAttributeSetClass), *GetNameSafe(Owner));
```

### LINE 1388
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Error,
    TEXT("InitializeFromDataAsset: Failed to initialize AttributeSet for owner '%s' "
         "— ASC missing or no AttributeSet class configured"),
    *GetNameSafe(Owner));
```

**AFTER:**
```cpp
PH_LOG_ERROR(LogStatsManager, "InitializeFromDataAsset: Failed to initialize AttributeSet for owner '%s' — ASC missing or no AttributeSet class configured",
    *GetNameSafe(Owner));
```

**NOTE:** Multi-line format string concatenated into single literal.

### LINE 1505
**BEFORE:**
```cpp
UE_LOG(LogStatsManager, Warning, TEXT("InitializeFromMap: Must be called on server"));
```

**AFTER:**
```cpp
PH_LOG_WARNING(LogStatsManager, "InitializeFromMap: Must be called on server");
```

---

## Private/Systems/Tags/Components/TagManager.cpp

(Note: No raw UE_LOG Warning/Error calls found with scope-limited grep in this file. Review manually if needed.)

---

## Summary Statistics

**Total non-compliant call sites: 68**

| File | Count |
|------|-------|
| DoTManager.cpp | 2 |
| EquipmentManager.cpp | 10 |
| EquipmentPresentationComponent.cpp | 8 |
| CharacterProgressionManager.cpp | 11 |
| StatsManager.cpp | 37 |
| **Total** | **68** |

---

## Conversion Checklist

- [ ] DoTManager.cpp: 2 calls
- [ ] EquipmentManager.cpp: 10 calls
- [ ] EquipmentPresentationComponent.cpp: 8 calls
- [ ] CharacterProgressionManager.cpp: 11 calls
- [ ] StatsManager.cpp: 37 calls
- [ ] Verify macros include required headers (`#include "Core/Logging/ProjectHunterLogMacros.h"`)
- [ ] Test compilation after conversion
- [ ] Verify runtime logging output format matches expectations
