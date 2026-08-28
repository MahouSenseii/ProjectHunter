#include "Combat/Components/CombatManager.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/Library/FunctionLibraries/PHSkillFunctionLibrary.h"
#include "AI/Components/MonsterModifierComponent.h"
#include "Combat/Components/UCombatStatusEffectApplier.h"
#include "Combat/Calculators/CombatOutgoingDamageCalculator.h"
#include "Combat/Resolvers/CombatIncomingDamageResolver.h"
#include "Combat/Library/FunctionLibraries/CombatFunctionLibrary.h"
#include "Equipment/Components/EquipmentManager.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "Item/ItemInstance.h"
#include "Stats/Library/FunctionLibraries/ItemLocalStatResolver.h"
#include "Stats/Library/FunctionLibraries/ContextualStatModifierEvaluator.h"
#include "Stats/Components/StatsManager.h"
#include "Tags/PHGameplayTags.h"

DEFINE_LOG_CATEGORY(LogCombatManager);

namespace CombatManagerPrivate
{
	FVector ResolveDamagePopupWorldLocation(const AActor* TargetActor)
	{
		if (!IsValid(TargetActor))
		{
			return FVector::ZeroVector;
		}

		return TargetActor->GetActorLocation() +
			FVector(0.f, 0.f, TargetActor->GetSimpleCollisionHalfHeight());
	}

	UItemInstance* ResolveAttackWeapon(
		const AActor* AttackerActor,
		const ECombatWeaponSource WeaponSource)
	{
		if (!IsValid(AttackerActor)
			|| WeaponSource == ECombatWeaponSource::CharacterAttributes)
		{
			return nullptr;
		}

		const UEquipmentManager* Equipment = AttackerActor->FindComponentByClass<UEquipmentManager>();
		if (!Equipment)
		{
			return nullptr;
		}

		auto GetWeapon = [Equipment](const EEquipmentSlot RequestedSlot)
		{
			const EEquipmentSlot OccupyingSlot = Equipment->ResolveOccupyingSlot(RequestedSlot);
			UItemInstance* Item = Equipment->GetEquippedItem(OccupyingSlot);
			return IsValid(Item) && Item->GetItemType() == EItemType::IT_Weapon
				? Item
				: nullptr;
		};

		switch (WeaponSource)
		{
		case ECombatWeaponSource::MainHand:
			return GetWeapon(EEquipmentSlot::ES_MainHand);
		case ECombatWeaponSource::OffHand:
			return GetWeapon(EEquipmentSlot::ES_OffHand);
		case ECombatWeaponSource::TwoHand:
			return GetWeapon(EEquipmentSlot::ES_TwoHand);
		case ECombatWeaponSource::Automatic:
		default:
			if (UItemInstance* TwoHand = GetWeapon(EEquipmentSlot::ES_TwoHand))
			{
				return TwoHand;
			}
			if (UItemInstance* MainHand = GetWeapon(EEquipmentSlot::ES_MainHand))
			{
				return MainHand;
			}
			return GetWeapon(EEquipmentSlot::ES_OffHand);
		}
	}
}

void UCombatIncomingHitEditContext::RejectHit()
{
	bApplyHit = false;
}

UCombatManager::UCombatManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// Owned sub-object, not a sibling actor component: CombatManager stays the
	// only actor component in this domain, and status effects still have one
	// clear owner via this instance rather than a FindComponentByClass search.
	CombatStatus = CreateDefaultSubobject<UCombatStatusEffectApplier>(TEXT("CombatStatus"));
}

UAbilitySystemComponent* UCombatManager::GetAbilitySystemComponentFromActor(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor))
	{
		return ASC;
	}

	return Actor->FindComponentByClass<UAbilitySystemComponent>();
}

const UHunterAttributeSet* UCombatManager::GetHunterAttributeSetFromActor(const AActor* Actor)
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActor(Actor);
	return ASC ? ASC->GetSet<UHunterAttributeSet>() : nullptr;
}

bool UCombatManager::ApplyHit(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const FAnimationDamageInfo& DamageInfo,
	FCombatResolveResult& OutResult,
	const EHitResponse HitResponse,
	const bool bCanApplyAilments)
{
	OutResult = FCombatResolveResult{};
	if (!IsValid(AttackerActor) || !IsValid(DefenderActor))
	{
		return false;
	}

	// Existing combat Blueprints sometimes call the manager obtained from the
	// hit/defender actor. Always resolve through the attacker-owned manager so
	// its effects, status tuning, authority channel, and duplicate tracking are used.
	if (GetOwner() != AttackerActor)
	{
		if (UCombatManager* AttackerManager = AttackerActor->FindComponentByClass<UCombatManager>())
		{
			if (AttackerManager != this)
			{
				return AttackerManager->ApplyHit(
					AttackerActor, DefenderActor, DamageInfo, OutResult,
					HitResponse, bCanApplyAilments);
			}
		}

		UE_LOG(LogCombatManager, Warning,
			TEXT("ApplyHit could not route to an attacker-owned CombatManager. InvokedOwner=%s Attacker=%s"),
			*GetNameSafe(GetOwner()), *GetNameSafe(AttackerActor));
		return false;
	}

	if (!AttackerActor->HasAuthority())
	{
		ServerApplyHit(DefenderActor, DamageInfo, HitResponse, bCanApplyAilments);
		return true;
	}

	FCombatHitContext HitContext = CreateCombatHitContext();
	return ApplyHitInternal(
		AttackerActor, DefenderActor, DamageInfo, HitContext,
		OutResult, HitResponse, bCanApplyAilments);
}

bool UCombatManager::ApplyHitWithContext(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const FAnimationDamageInfo& DamageInfo,
	const FCombatHitContext& HitContext,
	FCombatResolveResult& OutResult,
	const EHitResponse HitResponse,
	const bool bCanApplyAilments)
{
	OutResult = FCombatResolveResult{};
	if (!IsValid(AttackerActor) || !IsValid(DefenderActor))
	{
		return false;
	}

	if (GetOwner() != AttackerActor)
	{
		if (UCombatManager* AttackerManager = AttackerActor->FindComponentByClass<UCombatManager>())
		{
			if (AttackerManager != this)
			{
				return AttackerManager->ApplyHitWithContext(
					AttackerActor, DefenderActor, DamageInfo, HitContext, OutResult,
					HitResponse, bCanApplyAilments);
			}
		}

		UE_LOG(LogCombatManager, Warning,
			TEXT("ApplyHitWithContext could not route to an attacker-owned CombatManager. InvokedOwner=%s Attacker=%s"),
			*GetNameSafe(GetOwner()), *GetNameSafe(AttackerActor));
		return false;
	}

	if (!AttackerActor->HasAuthority())
	{
		ServerApplyHitWithContext(
			DefenderActor, DamageInfo, HitContext, HitResponse, bCanApplyAilments);
		return true;
	}

	FCombatHitContext EffectiveContext = HitContext;
	if (!EffectiveContext.AttackId.IsValid() || EffectiveContext.RandomSeed == 0)
	{
		const FCombatHitContext GeneratedContext = CreateCombatHitContext(EffectiveContext.RandomSeed);
		if (!EffectiveContext.AttackId.IsValid())
		{
			EffectiveContext.AttackId = GeneratedContext.AttackId;
		}
		if (EffectiveContext.RandomSeed == 0)
		{
			EffectiveContext.RandomSeed = GeneratedContext.RandomSeed;
		}
	}

	return ApplyHitInternal(
		AttackerActor, DefenderActor, DamageInfo, EffectiveContext,
		OutResult, HitResponse, bCanApplyAilments);
}

void UCombatManager::ServerApplyHit_Implementation(
	AActor* DefenderActor,
	const FAnimationDamageInfo& DamageInfo,
	const EHitResponse HitResponse,
	const bool bCanApplyAilments)
{
	FCombatResolveResult IgnoredResult;
	ApplyHit(GetOwner(), DefenderActor, DamageInfo, IgnoredResult, HitResponse, bCanApplyAilments);
}

void UCombatManager::ServerApplyHitWithContext_Implementation(
	AActor* DefenderActor,
	const FAnimationDamageInfo& DamageInfo,
	const FCombatHitContext& HitContext,
	const EHitResponse HitResponse,
	const bool bCanApplyAilments)
{
	FCombatResolveResult IgnoredResult;
	ApplyHitWithContext(
		GetOwner(), DefenderActor, DamageInfo, HitContext, IgnoredResult,
		HitResponse, bCanApplyAilments);
}

void UCombatManager::ClientReceiveDamagePopup_Implementation(
	const FCombatDamagePopupData& PopupData)
{
	OnDamagePopupRequested.Broadcast(PopupData);
}

FCombatHitContext UCombatManager::CreateCombatHitContext(const int32 RandomSeed)
{
	FCombatHitContext Context;
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !Owner->HasAuthority())
	{
		UE_LOG(LogCombatManager, Warning,
			TEXT("CreateCombatHitContext rejected on non-authority owner %s."), *GetNameSafe(Owner));
		return Context;
	}

	Context.AttackId = FGuid::NewGuid();
	++NextAttackSequence;
	const uint32 GeneratedSeed = HashCombineFast(GetTypeHash(Context.AttackId), NextAttackSequence);
	Context.RandomSeed = RandomSeed != 0
		? RandomSeed
		: static_cast<int32>((GeneratedSeed & MAX_int32) != 0 ? (GeneratedSeed & MAX_int32) : 1u);
	return Context;
}

void UCombatManager::ClearCombatHitContext(const FGuid& AttackId)
{
	if (!AttackId.IsValid())
	{
		return;
	}

	ProcessedTargetsByAttack.Remove(AttackId);
	RememberedAttackOrder.RemoveSingleSwap(AttackId, EAllowShrinking::No);
}

bool UCombatManager::ValidateAuthoritativeHit(AActor* AttackerActor, AActor* DefenderActor) const
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !Owner->HasAuthority() || !IsValid(AttackerActor) || !IsValid(DefenderActor))
	{
		UE_LOG(LogCombatManager, Warning,
			TEXT("ApplyHit requires valid authority actors. Owner=%s Attacker=%s Defender=%s"),
			*GetNameSafe(Owner), *GetNameSafe(AttackerActor), *GetNameSafe(DefenderActor));
		return false;
	}

	if (Owner != AttackerActor)
	{
		UE_LOG(LogCombatManager, Warning,
			TEXT("ApplyHit rejected because CombatManager owner %s is not attacker %s."),
			*GetNameSafe(Owner), *GetNameSafe(AttackerActor));
		return false;
	}

	if (!DefenderActor->HasAuthority() || AttackerActor == DefenderActor)
	{
		return false;
	}

	const FGameplayTag DeadTag = FPHGameplayTags::Get().Condition_Dead;
	const UAbilitySystemComponent* AttackerASC = GetAbilitySystemComponentFromActor(AttackerActor);
	const UAbilitySystemComponent* DefenderASC = GetAbilitySystemComponentFromActor(DefenderActor);
	if ((AttackerASC && AttackerASC->HasMatchingGameplayTag(DeadTag))
		|| (DefenderASC && DefenderASC->HasMatchingGameplayTag(DeadTag)))
	{
		return false;
	}

	return true;
}

bool UCombatManager::HasAlreadyProcessedTarget(
	const FCombatHitContext& HitContext,
	AActor* DefenderActor) const
{
	if (!HitContext.AttackId.IsValid() || HitContext.bAllowRepeatHit || !IsValid(DefenderActor))
	{
		return false;
	}

	const TSet<TWeakObjectPtr<AActor>>* Targets = ProcessedTargetsByAttack.Find(HitContext.AttackId);
	return Targets && Targets->Contains(DefenderActor);
}

void UCombatManager::RememberProcessedTarget(
	const FCombatHitContext& HitContext,
	AActor* DefenderActor)
{
	if (!HitContext.AttackId.IsValid() || HitContext.bAllowRepeatHit || !IsValid(DefenderActor))
	{
		return;
	}

	if (!ProcessedTargetsByAttack.Contains(HitContext.AttackId))
	{
		while (RememberedAttackOrder.Num() >= MaxRememberedAttackContexts)
		{
			const FGuid OldestAttackId = RememberedAttackOrder[0];
			RememberedAttackOrder.RemoveAt(0, EAllowShrinking::No);
			ProcessedTargetsByAttack.Remove(OldestAttackId);
		}
		RememberedAttackOrder.Add(HitContext.AttackId);
	}

	ProcessedTargetsByAttack.FindOrAdd(HitContext.AttackId).Add(DefenderActor);
}

int32 UCombatManager::ResolveHitSeed(
	const FCombatHitContext& HitContext,
	const AActor* DefenderActor) const
{
	uint32 Seed = HitContext.RandomSeed != 0
		? static_cast<uint32>(HitContext.RandomSeed)
		: GetTypeHash(HitContext.AttackId);
	Seed = HashCombineFast(Seed, GetTypeHash(GetPathNameSafe(DefenderActor)));
	Seed = HashCombineFast(Seed, static_cast<uint32>(FMath::Max(0, HitContext.HitIndex)));
	const int32 ResolvedSeed = static_cast<int32>(Seed & MAX_int32);
	return ResolvedSeed != 0 ? ResolvedSeed : 1;
}

bool UCombatManager::ApplyHitInternal(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const FAnimationDamageInfo& DamageInfo,
	const FCombatHitContext& HitContext,
	FCombatResolveResult& OutResult,
	const EHitResponse HitResponse,
	const bool bCanApplyAilments)
{
	OutResult = FCombatResolveResult{};

	if (!ValidateAuthoritativeHit(AttackerActor, DefenderActor))
	{
		return false;
	}

	if (HasAlreadyProcessedTarget(HitContext, DefenderActor))
	{
		UE_LOG(LogCombatManager, VeryVerbose,
			TEXT("ApplyHit ignored duplicate target %s for attack %s."),
			*GetNameSafe(DefenderActor), *HitContext.AttackId.ToString());
		return false;
	}

	const UHunterAttributeSet* AttackerAttributes = GetHunterAttributeSetFromActor(AttackerActor);
	const UHunterAttributeSet* DefenderAttributes = GetHunterAttributeSetFromActor(DefenderActor);
	if (!AttackerAttributes || !DefenderAttributes)
	{
		UE_LOG(LogCombatManager, Warning,
			TEXT("ApplyHit requires a UHunterAttributeSet on both actors. Attacker=%s(%s) Defender=%s(%s)"),
			*GetNameSafe(AttackerActor), AttackerAttributes ? TEXT("ok") : TEXT("missing"),
			*GetNameSafe(DefenderActor), DefenderAttributes ? TEXT("ok") : TEXT("missing"));
		return false;
	}

	FAnimationDamageInfo EffectiveInfo = DamageInfo;
	FPHSkillDataResolver::MergeSkillTagsIntoDamageInfo(EffectiveInfo, HitContext.SkillTags);
	EHitResponse EffectiveHitResponse = HitResponse;
	bool bEffectiveCanApplyAilments = bCanApplyAilments;

	if (OnEditIncomingHit.IsBound())
	{
		UCombatIncomingHitEditContext* EditContext = NewObject<UCombatIncomingHitEditContext>(this);
		EditContext->AttackerActor = AttackerActor;
		EditContext->DefenderActor = DefenderActor;
		EditContext->HitContext = HitContext;
		EditContext->DamageInfo = EffectiveInfo;
		EditContext->HitResponse = EffectiveHitResponse;
		EditContext->bCanApplyAilments = bEffectiveCanApplyAilments;

		OnEditIncomingHit.Broadcast(EditContext);

		if (!EditContext->bApplyHit)
		{
			UE_LOG(LogCombatManager, Verbose,
				TEXT("ApplyHit rejected by incoming hit edit. Attacker=%s Defender=%s"),
				*GetNameSafe(AttackerActor), *GetNameSafe(DefenderActor));
			return false;
		}

		EffectiveInfo = EditContext->DamageInfo;
		EffectiveHitResponse = EditContext->HitResponse;
		bEffectiveCanApplyAilments = EditContext->bCanApplyAilments;
	}

	// Defensive outcome is the defender's to declare, not the attacker's.
	// This runs after the edit hook so neither a caller nor a listener can
	// assert a parry or i-frame the defender does not actually have.
	{
		bool bOverrodeCaller = false;
		const EHitResponse AuthoritativeResponse =
			FCombatIncomingDamageResolver::ResolveDefenderHitResponse(
				DefenderActor, EffectiveHitResponse, bOverrodeCaller);

		if (bOverrodeCaller)
		{
			UE_LOG(LogCombatManager, Verbose,
				TEXT("ApplyHit hit response resolved from defender state: requested=%s authoritative=%s (Attacker=%s Defender=%s)"),
				*UEnum::GetDisplayValueAsText(EffectiveHitResponse).ToString(),
				*UEnum::GetDisplayValueAsText(AuthoritativeResponse).ToString(),
				*GetNameSafe(AttackerActor), *GetNameSafe(DefenderActor));
		}

		EffectiveHitResponse = AuthoritativeResponse;
	}

	RememberProcessedTarget(HitContext, DefenderActor);
	FRandomStream RandomStream(ResolveHitSeed(HitContext, DefenderActor));

	FStatModifierEvaluationContext ModifierContext;
	ModifierContext.bIsSkillHit = true;
	ModifierContext.SourceTags = HitContext.SkillTags;
	const UAbilitySystemComponent* AttackerTagASC = GetAbilitySystemComponentFromActor(AttackerActor);
	const UAbilitySystemComponent* DefenderTagASC = GetAbilitySystemComponentFromActor(DefenderActor);
	if (AttackerTagASC)
	{
		FGameplayTagContainer OwnedSourceTags;
		AttackerTagASC->GetOwnedGameplayTags(OwnedSourceTags);
		ModifierContext.SourceTags.AppendTags(OwnedSourceTags);
	}
	if (DefenderTagASC)
	{
		DefenderTagASC->GetOwnedGameplayTags(ModifierContext.TargetTags);
	}
	const float MaxSourceHealth = FMath::Max(
		AttackerAttributes->GetMaxEffectiveHealth(), AttackerAttributes->GetMaxHealth());
	ModifierContext.SourceHealthPercent = MaxSourceHealth > 0.f
		? FMath::Clamp(AttackerAttributes->GetHealth() / MaxSourceHealth, 0.f, 1.f)
		: 1.f;
	ModifierContext.bIsMoving = !AttackerActor->GetVelocity().IsNearlyZero();

	TArray<UItemInstance*> EquippedItems;
	if (const UEquipmentManager* Equipment = AttackerActor->FindComponentByClass<UEquipmentManager>())
	{
		EquippedItems = Equipment->GetAllEquippedItems();
		const UItemInstance* MainHand = Equipment->GetEquippedItem(EEquipmentSlot::ES_MainHand);
		const UItemInstance* OffHand = Equipment->GetEquippedItem(EEquipmentSlot::ES_OffHand);
		const UItemInstance* TwoHand = Equipment->GetEquippedItem(EEquipmentSlot::ES_TwoHand);
		const bool bMainWeapon = IsValid(MainHand) && MainHand->GetItemType() == EItemType::IT_Weapon;
		const bool bOffWeapon = IsValid(OffHand) && OffHand->GetItemType() == EItemType::IT_Weapon;
		ModifierContext.bIsDualWielding = bMainWeapon && bOffWeapon;
		ModifierContext.bIsUnarmed = !bMainWeapon && !bOffWeapon && !IsValid(TwoHand);
		ModifierContext.bHasShield = IsValid(OffHand)
			&& OffHand->GetItemSubType() == EItemSubType::IST_Shield;
	}
	else
	{
		ModifierContext.bIsUnarmed = true;
	}

	const UStatsManager* StatsManager = AttackerActor->FindComponentByClass<UStatsManager>();
	const FContextualStatModifierSnapshot ContextualModifiers =
		FContextualStatModifierEvaluator::BuildFromItems(
			EquippedItems, StatsManager, ModifierContext);
	const FContextualStatModifierSnapshot* ContextualModifierPtr =
		ContextualModifiers.IsEmpty() ? nullptr : &ContextualModifiers;

	FResolvedWeaponStats ResolvedWeaponStats;
	const UItemInstance* AttackWeapon = CombatManagerPrivate::ResolveAttackWeapon(
		AttackerActor, EffectiveInfo.WeaponSource);
	const FResolvedWeaponStats* WeaponStats =
		FItemLocalStatResolver::ResolveWeapon(AttackWeapon, ResolvedWeaponStats)
			? &ResolvedWeaponStats
			: nullptr;

	// Outgoing: base -> conversion -> increased/more -> crit.
	FCombatDamagePacket OutgoingPacket = FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(
		AttackerAttributes, EffectiveInfo, RandomStream, WeaponStats, ContextualModifierPtr);
	if (const UMonsterModifierComponent* MonsterModifiers =
		AttackerActor->FindComponentByClass<UMonsterModifierComponent>())
	{
		OutgoingPacket.Scale(MonsterModifiers->GetCombinedDamageMultiplier());
	}
	// Positional bonus: rear hits hurt more. Applied after scaling and crit but
	// before mitigation, so armour and resistances still work against the
	// boosted number. This is a damage modifier only - not a backstab execution.
	const EHitDirection HitDirection =
		UCombatFunctionLibrary::GetHitDirection(AttackerActor, DefenderActor, PositionalRules);
	const float PositionalMultiplier =
		UCombatFunctionLibrary::GetPositionalDamageMultiplier(HitDirection, PositionalRules);
	if (!FMath::IsNearlyEqual(PositionalMultiplier, 1.f))
	{
		OutgoingPacket.Scale(PositionalMultiplier);
	}

	UE_LOG(LogCombatManager, Verbose, TEXT("ApplyHit outgoing packet: %s (direction=%s x%.2f)"),
		*FCombatOutgoingDamageCalculator::FormatPacket(OutgoingPacket),
		*UEnum::GetDisplayValueAsText(HitDirection).ToString(),
		PositionalMultiplier);

	if (OutgoingPacket.TotalPreMitigation <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(
			LogCombatManager,
			Warning,
			TEXT("ApplyHit resolved zero outgoing damage. Attacker=%s Defender=%s WeaponPhysical=%.2f-%.2f WeaponEffectiveness=%.1f%% GlobalMore=%.3f PhysicalMore=%.3f. Check the attack DamageInfo and base-stat multiplier defaults."),
			*GetNameSafe(AttackerActor),
			*GetNameSafe(DefenderActor),
			WeaponStats ? WeaponStats->Values.MinPhysicalDamage : AttackerAttributes->GetMinPhysicalDamage(),
			WeaponStats ? WeaponStats->Values.MaxPhysicalDamage : AttackerAttributes->GetMaxPhysicalDamage(),
			EffectiveInfo.WeaponDamageEffectivenessPercent,
			AttackerAttributes->GetGlobalMoreDamage(),
			AttackerAttributes->GetPhysicalMoreDamage());
	}

	// Incoming: armour/resists -> block -> taken multipliers -> routing.
	OutResult = FCombatIncomingDamageResolver::MitigateDamagePacket(
		OutgoingPacket, AttackerActor, DefenderActor,
		AttackerAttributes, DefenderAttributes, EffectiveInfo);
	OutResult.HitDirection = HitDirection;
	OutResult.PositionalMultiplierApplied = PositionalMultiplier;

	// Stagger and hit-response gates run before application so both the
	// authoritative path and non-authority previews agree on the outcome.
	FCombatIncomingDamageResolver::EvaluateStagger(
		DefenderActor, DefenderAttributes, EffectiveInfo, OutResult);
	FCombatIncomingDamageResolver::ApplyHitResponse(EffectiveHitResponse, bEffectiveCanApplyAilments, OutResult);

	UAbilitySystemComponent* AttackerASC = GetAbilitySystemComponentFromActor(AttackerActor);

	ApplyResolvedDamage(AttackerActor, DefenderActor, OutResult);
	OutResult.bKilledTarget = OutResult.HitResponse != EHitResponse::Invincible
		&& DefenderAttributes->GetHealth() <= 0.f;
	OutResult.HealthAfterHit = DefenderAttributes->GetHealth();

	ApplyOnHitRecovery(AttackerActor, OutResult, AttackerASC, AttackerAttributes);
	ApplyAilments(AttackerActor, DefenderActor, OutResult, RandomStream);
	ApplyReflect(AttackerActor, DefenderActor, OutResult, RandomStream);

	BroadcastDamagePopup(AttackerActor, DefenderActor, OutResult);

	UE_LOG(LogCombatManager, Verbose, TEXT("ApplyHit completed. Attacker=%s Defender=%s %s"),
		*GetNameSafe(AttackerActor), *GetNameSafe(DefenderActor),
		*FCombatIncomingDamageResolver::FormatResult(OutResult));

	return true;
}

// Application

void UCombatManager::ApplyResolvedDamage(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const FCombatResolveResult& Result) const
{
	if (!IsValid(DefenderActor) || !DefenderActor->HasAuthority())
	{
		return;
	}

	if (Result.TotalDamageApplied <= 0.f && Result.DamageToStamina <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* DefenderASC = GetAbilitySystemComponentFromActor(DefenderActor);
	const UHunterAttributeSet* DefenderAttributes = GetHunterAttributeSetFromActor(DefenderActor);
	if (!DefenderASC || !DefenderAttributes)
	{
		return;
	}

	if (DamageApplicationGE)
	{
		UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActor(AttackerActor);
		UAbilitySystemComponent* SpecOwnerASC = SourceASC ? SourceASC : DefenderASC;
		FGameplayEffectContextHandle Context = SpecOwnerASC->MakeEffectContext();
		Context.AddInstigator(AttackerActor, AttackerActor);
		Context.AddSourceObject(AttackerActor ? AttackerActor : GetOwner());

		const FGameplayEffectSpecHandle Spec = SpecOwnerASC->MakeOutgoingSpec(DamageApplicationGE, 1.f, Context);
		if (!Spec.IsValid())
		{
			UE_LOG(LogCombatManager, Error,
				TEXT("ApplyResolvedDamage: MakeOutgoingSpec failed for DamageApplicationGE on %s."),
				*GetNameSafe(this));
			return;
		}

		const FPHGameplayTags& Tags = FPHGameplayTags::Get();
		if (Result.DamageToHealth > 0.f)
		{
			Spec.Data->SetSetByCallerMagnitude(Tags.Data_Damage_Health, -Result.DamageToHealth);
		}
		if (Result.DamageToArcaneShield > 0.f)
		{
			Spec.Data->SetSetByCallerMagnitude(Tags.Data_Damage_ArcaneShield, -Result.DamageToArcaneShield);
		}
		if (Result.DamageToStamina > 0.f)
		{
			Spec.Data->SetSetByCallerMagnitude(Tags.Data_Damage_Stamina, -Result.DamageToStamina);
		}

		SpecOwnerASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), DefenderASC);
		return;
	}

	// Fallback path bypasses GAS clamping; configure DamageApplicationGE.
	UE_LOG(LogCombatManager, Warning,
		TEXT("ApplyResolvedDamage: DamageApplicationGE is not set on %s. Falling back to SetNumericAttributeBase."),
		*GetNameSafe(this));

	const float NewStamina = FMath::Max(
		0.f, DefenderAttributes->GetStamina() - FMath::Max(Result.DamageToStamina, 0.f));
	const float NewArcaneShield = FMath::Max(
		0.f, DefenderAttributes->GetArcaneShield() - FMath::Max(Result.DamageToArcaneShield, 0.f));
	const float NewHealth = FMath::Max(
		0.f, DefenderAttributes->GetHealth() - FMath::Max(Result.DamageToHealth, 0.f));

	DefenderASC->SetNumericAttributeBase(UHunterAttributeSet::GetStaminaAttribute(), NewStamina);
	DefenderASC->SetNumericAttributeBase(UHunterAttributeSet::GetArcaneShieldAttribute(), NewArcaneShield);
	DefenderASC->SetNumericAttributeBase(UHunterAttributeSet::GetHealthAttribute(), NewHealth);
}

void UCombatManager::ApplyOnHitRecovery(
	AActor* AttackerActor,
	const FCombatResolveResult& Result,
	UAbilitySystemComponent* AttackerASC,
	const UHunterAttributeSet* AttackerAttributes) const
{
	if (!AttackerASC || !AttackerAttributes || Result.TotalDamageApplied <= 0.f)
	{
		return;
	}

	const float HealthRecovery = FMath::Max(0.f, AttackerAttributes->GetLifeOnHit())
		+ FMath::Max(0.f, Result.TotalDamageApplied * (AttackerAttributes->GetLifeLeech() / 100.f));
	const float ManaRecovery = FMath::Max(0.f, AttackerAttributes->GetManaOnHit())
		+ FMath::Max(0.f, Result.TotalDamageApplied * (AttackerAttributes->GetManaLeech() / 100.f));
	const float StaminaRecovery = FMath::Max(0.f, AttackerAttributes->GetStaminaOnHit())
		+ FMath::Max(0.f, Result.TotalDamageApplied * (AttackerAttributes->GetStaminaLeechPercent() / 100.f));

	if (HealthRecovery <= 0.f && ManaRecovery <= 0.f && StaminaRecovery <= 0.f)
	{
		return;
	}

	if (!RecoveryApplicationGE)
	{
		UE_LOG(LogCombatManager, Warning,
			TEXT("ApplyOnHitRecovery: RecoveryApplicationGE is not set on %s; on-hit recovery skipped."),
			*GetNameSafe(this));
		return;
	}

	FGameplayEffectContextHandle Context = AttackerASC->MakeEffectContext();
	Context.AddSourceObject(AttackerActor);

	const FGameplayEffectSpecHandle Spec = AttackerASC->MakeOutgoingSpec(RecoveryApplicationGE, 1.f, Context);
	if (!Spec.IsValid())
	{
		return;
	}

	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	if (HealthRecovery > 0.f)
	{
		Spec.Data->SetSetByCallerMagnitude(Tags.Data_Recovery_Health, HealthRecovery);
	}
	if (ManaRecovery > 0.f)
	{
		Spec.Data->SetSetByCallerMagnitude(Tags.Data_Recovery_Mana, ManaRecovery);
	}
	if (StaminaRecovery > 0.f)
	{
		Spec.Data->SetSetByCallerMagnitude(Tags.Data_Recovery_Stamina, StaminaRecovery);
	}

	AttackerASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void UCombatManager::ApplyAilments(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const FCombatResolveResult& Result,
	FRandomStream& RandomStream) const
{
	if (!Result.bShouldApplyAilments || Result.HitResponse == EHitResponse::Invincible)
	{
		return;
	}

	const UHunterAttributeSet* AttackerAttributes = GetHunterAttributeSetFromActor(AttackerActor);
	if (!AttackerAttributes || !IsValid(DefenderActor))
	{
		return;
	}

	UCombatStatusEffectApplier* StatusManager = CombatStatus;
	if (!StatusManager)
	{
		UE_LOG(LogCombatManager, Verbose,
			TEXT("ApplyAilments skipped: no CombatStatus effect applier found for %s."),
			*GetNameSafe(AttackerActor));
		return;
	}

	const auto RollChance = [&RandomStream](const float ChancePercent) -> bool
	{
		return ChancePercent > 0.f
			&& RandomStream.FRandRange(0.f, 100.f) < FMath::Min(ChancePercent, 100.f);
	};
	const auto ResolveDuration = [](const float AttributeDuration, const float DefaultDuration) -> float
	{
		return AttributeDuration > 0.f ? AttributeDuration : DefaultDuration;
	};

	// Each typed ailment requires that damage type to have actually landed.
	// A successful parry zeroes every per-type taken value and clears
	// bShouldApplyAilments, so nothing below can fire through a parry.
	if (Result.PhysicalTaken > 0.f && RollChance(AttackerAttributes->GetChanceToBleed()))
	{
		StatusManager->ApplyBleed(
			DefenderActor,
			Result.PhysicalTaken * AilmentTuning.BleedDamagePerTickFraction,
			ResolveDuration(AttackerAttributes->GetBleedDuration(), AilmentTuning.BleedDuration),
			AttackerActor);
	}

	if (Result.FireTaken > 0.f && RollChance(AttackerAttributes->GetChanceToIgnite()))
	{
		StatusManager->ApplyIgnite(
			DefenderActor,
			Result.FireTaken * AilmentTuning.IgniteDamagePerTickFraction,
			ResolveDuration(AttackerAttributes->GetBurnDuration(), AilmentTuning.IgniteDuration),
			AttackerActor);
	}

	if (Result.CorruptionTaken > 0.f && RollChance(AttackerAttributes->GetChanceToCorrupt()))
	{
		StatusManager->ApplyCorruption(
			DefenderActor,
			Result.CorruptionTaken * AilmentTuning.CorruptionDamagePerTickFraction,
			ResolveDuration(AttackerAttributes->GetCorruptionDuration(), AilmentTuning.CorruptionDuration),
			AttackerActor);
	}

	if (Result.IceTaken > 0.f && AilmentTuning.bColdDamageAlwaysChills)
	{
		// Cold damage always chills; freeze is the chance roll on top.
		StatusManager->ApplyChill(
			DefenderActor,
			AilmentTuning.ChillSlowFraction,
			AilmentTuning.ChillDuration,
			AttackerActor);
	}

	if (Result.IceTaken > 0.f && RollChance(AttackerAttributes->GetChanceToFreeze()))
	{
		StatusManager->ApplyFreeze(
			DefenderActor,
			ResolveDuration(AttackerAttributes->GetFreezeDuration(), AilmentTuning.FreezeDuration),
			AttackerActor);
	}

	if (Result.LightningTaken > 0.f && RollChance(AttackerAttributes->GetChanceToShock()))
	{
		StatusManager->ApplyShock(
			DefenderActor,
			AilmentTuning.ShockDamageTakenFraction,
			ResolveDuration(AttackerAttributes->GetShockDuration(), AilmentTuning.ShockDuration),
			AttackerActor);
	}

	if (Result.LightTaken > 0.f && RollChance(AttackerAttributes->GetChanceToPetrify()))
	{
		StatusManager->ApplyPetrify(
			DefenderActor,
			AilmentTuning.PetrifyDuration,
			AttackerActor);
	}
}

void UCombatManager::ApplyReflect(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const FCombatResolveResult& Result,
	FRandomStream& RandomStream) const
{
	if (Result.TotalDamageTaken <= 0.f || !IsValid(AttackerActor) || !IsValid(DefenderActor))
	{
		return;
	}

	const UHunterAttributeSet* DefenderAttributes = GetHunterAttributeSetFromActor(DefenderActor);
	if (!DefenderAttributes)
	{
		return;
	}

	const auto RollChance = [&RandomStream](const float ChancePercent) -> bool
	{
		return ChancePercent > 0.f
			&& RandomStream.FRandRange(0.f, 100.f) < FMath::Min(ChancePercent, 100.f);
	};

	float ReflectedDamage = 0.f;
	if (Result.PhysicalTaken > 0.f && RollChance(DefenderAttributes->GetReflectChancePhysical()))
	{
		ReflectedDamage += Result.PhysicalTaken * (FMath::Max(0.f, DefenderAttributes->GetReflectPhysical()) / 100.f);
	}

	const float ElementalTaken =
		Result.FireTaken + Result.IceTaken + Result.LightningTaken + Result.LightTaken;
	if (ElementalTaken > 0.f && RollChance(DefenderAttributes->GetReflectChanceElemental()))
	{
		ReflectedDamage += ElementalTaken * (FMath::Max(0.f, DefenderAttributes->GetReflectElemental()) / 100.f);
	}

	const float AppliedDamageScale = Result.TotalDamageTaken > 0.f
		? FMath::Clamp(Result.TotalDamageApplied / Result.TotalDamageTaken, 0.f, 1.f)
		: 0.f;
	ReflectedDamage *= AppliedDamageScale;

	if (ReflectedDamage <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* AttackerASC = GetAbilitySystemComponentFromActor(AttackerActor);
	UAbilitySystemComponent* DefenderASC = GetAbilitySystemComponentFromActor(DefenderActor);
	if (!AttackerASC || !DefenderASC)
	{
		return;
	}

	if (!ReflectApplicationGE)
	{
		UE_LOG(LogCombatManager, Warning,
			TEXT("ApplyReflect: %.2f reflect damage rolled but ReflectApplicationGE is not set on %s."),
			ReflectedDamage, *GetNameSafe(this));
		return;
	}

	FGameplayEffectContextHandle Context = DefenderASC->MakeEffectContext();
	Context.AddInstigator(DefenderActor, DefenderActor);
	Context.AddSourceObject(DefenderActor);

	const FGameplayEffectSpecHandle Spec = DefenderASC->MakeOutgoingSpec(ReflectApplicationGE, 1.f, Context);
	if (!Spec.IsValid())
	{
		return;
	}

	Spec.Data->SetSetByCallerMagnitude(FPHGameplayTags::Get().Data_Damage_Health, -ReflectedDamage);
	DefenderASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), AttackerASC);
}

void UCombatManager::BroadcastDamagePopup(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const FCombatResolveResult& Result)
{
	if (Result.TotalDamageTaken <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	FCombatDamagePopupData PopupData;
	PopupData.SourceActor = AttackerActor;
	PopupData.TargetActor = DefenderActor;
	PopupData.ResolveResult = Result;
	PopupData.TotalDamage = Result.TotalDamageTaken;
	PopupData.DominantDamageType = UCombatFunctionLibrary::GetDominantDamageTypeFromResolveResult(Result);
	PopupData.DisplayColor = UCombatFunctionLibrary::GetDefaultDamageTypeColor(PopupData.DominantDamageType);
	PopupData.WorldLocation = CombatManagerPrivate::ResolveDamagePopupWorldLocation(DefenderActor);
	PopupData.bWasCrit = Result.bWasCrit;
	PopupData.bWasBlocked = Result.bWasBlocked;
	PopupData.bKilledTarget = Result.bKilledTarget;

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const APlayerController* OwnerPlayerController = OwnerPawn
		? Cast<APlayerController>(OwnerPawn->GetController())
		: nullptr;

	// Hit resolution stays authoritative, but presentation belongs to the
	// attacking player. A remote player's server-side component has no local UI
	// listener, so send the already-resolved cosmetic payload to its owning client.
	if (GetOwner() && GetOwner()->HasAuthority()
		&& OwnerPlayerController && !OwnerPlayerController->IsLocalController())
	{
		ClientReceiveDamagePopup(PopupData);
		return;
	}

	OnDamagePopupRequested.Broadcast(PopupData);
}
