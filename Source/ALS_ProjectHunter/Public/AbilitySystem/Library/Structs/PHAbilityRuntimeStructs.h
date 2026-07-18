#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "AbilitySystem/Library/Enums/PHAbilityEnums.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayEffectTypes.h"

struct ALS_PROJECTHUNTER_API FPHAbilityActivationGroupRuntimeState
{
	FPHAbilityActivationGroupRuntimeState();

	void Reset();
	bool IsActivationGroupBlocked(EPHAbilityActivationGroup ActivationGroup) const;
	void AddAbilityToActivationGroup(EPHAbilityActivationGroup ActivationGroup);
	void RemoveAbilityFromActivationGroup(EPHAbilityActivationGroup ActivationGroup);
	int32 GetActivationGroupCount(EPHAbilityActivationGroup ActivationGroup) const;
	int32 GetExclusiveActivationGroupCount() const;

private:
	static constexpr uint8 ActivationGroupCount = static_cast<uint8>(EPHAbilityActivationGroup::MAX);

	int32 ActivationGroupCounts[ActivationGroupCount];
};

struct ALS_PROJECTHUNTER_API FPHPassiveResourceEffectRuntimeState
{
	FGameplayEffectSpecHandle HealthRegenSpec;
	FGameplayEffectSpecHandle ManaRegenSpec;
	FGameplayEffectSpecHandle StaminaRegenSpec;
	FGameplayEffectSpecHandle ArcaneShieldRegenSpec;

	void AddActiveEffectHandle(const FActiveGameplayEffectHandle& Handle);
	void ResetActiveEffectHandles();
	void ResetSpecs();
	void Reset();
	const TArray<FActiveGameplayEffectHandle>& GetActiveEffectHandles() const;

private:
	TArray<FActiveGameplayEffectHandle> ActiveEffectHandles;
};

struct ALS_PROJECTHUNTER_API FPHAbilityInputRuntimeState
{
	void AddPressedSpecHandle(const FGameplayAbilitySpecHandle& Handle);
	void AddReleasedSpecHandle(const FGameplayAbilitySpecHandle& Handle);
	void AddHeldSpecHandle(const FGameplayAbilitySpecHandle& Handle);
	void RemoveHeldSpecHandle(const FGameplayAbilitySpecHandle& Handle);
	void ResetFrameInput();
	void Reset();

	const TArray<FGameplayAbilitySpecHandle>& GetPressedSpecHandles() const;
	const TArray<FGameplayAbilitySpecHandle>& GetReleasedSpecHandles() const;
	const TArray<FGameplayAbilitySpecHandle>& GetHeldSpecHandles() const;

private:
	TArray<FGameplayAbilitySpecHandle> PressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> ReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> HeldSpecHandles;
};

struct ALS_PROJECTHUNTER_API FPHStaminaDegenRequestRuntimeState
{
	void SetSprintRequested(bool bRequested);
	void SetWallRunningRequested(bool bRequested);
	void Clear();

	bool IsSprintRequested() const;
	bool IsWallRunningRequested() const;
	bool HasAnyRequest() const;

private:
	bool bSprintRequested = false;
	bool bWallRunningRequested = false;
};
