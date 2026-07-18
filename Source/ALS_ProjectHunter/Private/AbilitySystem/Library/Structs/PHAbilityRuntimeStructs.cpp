#include "AbilitySystem/Library/Structs/PHAbilityRuntimeStructs.h"

#include "AbilitySystem/Library/FunctionLibraries/PHAbilitySystemFunctionLibrary.h"

FPHAbilityActivationGroupRuntimeState::FPHAbilityActivationGroupRuntimeState()
{
	Reset();
}

void FPHAbilityActivationGroupRuntimeState::Reset()
{
	FMemory::Memzero(ActivationGroupCounts, sizeof(ActivationGroupCounts));
}

bool FPHAbilityActivationGroupRuntimeState::IsActivationGroupBlocked(const EPHAbilityActivationGroup ActivationGroup) const
{
	if (UPHAbilitySystemFunctionLibrary::IsIndependentActivationGroup(ActivationGroup))
	{
		return false;
	}

	if (UPHAbilitySystemFunctionLibrary::IsExclusiveActivationGroup(ActivationGroup))
	{
		return GetActivationGroupCount(EPHAbilityActivationGroup::Exclusive_Blocking) > 0;
	}

	checkf(false, TEXT("IsActivationGroupBlocked: Invalid ActivationGroup [%d]."), static_cast<uint8>(ActivationGroup));
	return false;
}

void FPHAbilityActivationGroupRuntimeState::AddAbilityToActivationGroup(const EPHAbilityActivationGroup ActivationGroup)
{
	checkf(
		UPHAbilitySystemFunctionLibrary::IsValidActivationGroup(ActivationGroup),
		TEXT("AddAbilityToActivationGroup: Invalid ActivationGroup [%d]."),
		static_cast<uint8>(ActivationGroup));

	int32& GroupCount = ActivationGroupCounts[static_cast<uint8>(ActivationGroup)];
	check(GroupCount < INT32_MAX);
	GroupCount++;
}

void FPHAbilityActivationGroupRuntimeState::RemoveAbilityFromActivationGroup(const EPHAbilityActivationGroup ActivationGroup)
{
	checkf(
		UPHAbilitySystemFunctionLibrary::IsValidActivationGroup(ActivationGroup),
		TEXT("RemoveAbilityFromActivationGroup: Invalid ActivationGroup [%d]."),
		static_cast<uint8>(ActivationGroup));

	int32& GroupCount = ActivationGroupCounts[static_cast<uint8>(ActivationGroup)];
	if (ensure(GroupCount > 0))
	{
		GroupCount--;
	}
}

int32 FPHAbilityActivationGroupRuntimeState::GetActivationGroupCount(const EPHAbilityActivationGroup ActivationGroup) const
{
	checkf(
		UPHAbilitySystemFunctionLibrary::IsValidActivationGroup(ActivationGroup),
		TEXT("GetActivationGroupCount: Invalid ActivationGroup [%d]."),
		static_cast<uint8>(ActivationGroup));
	return ActivationGroupCounts[static_cast<uint8>(ActivationGroup)];
}

int32 FPHAbilityActivationGroupRuntimeState::GetExclusiveActivationGroupCount() const
{
	return GetActivationGroupCount(EPHAbilityActivationGroup::Exclusive_Replaceable) +
		GetActivationGroupCount(EPHAbilityActivationGroup::Exclusive_Blocking);
}

void FPHPassiveResourceEffectRuntimeState::AddActiveEffectHandle(const FActiveGameplayEffectHandle& Handle)
{
	if (Handle.IsValid())
	{
		ActiveEffectHandles.Add(Handle);
	}
}

void FPHPassiveResourceEffectRuntimeState::ResetActiveEffectHandles()
{
	ActiveEffectHandles.Reset();
}

void FPHPassiveResourceEffectRuntimeState::ResetSpecs()
{
	HealthRegenSpec = FGameplayEffectSpecHandle();
	ManaRegenSpec = FGameplayEffectSpecHandle();
	StaminaRegenSpec = FGameplayEffectSpecHandle();
	ArcaneShieldRegenSpec = FGameplayEffectSpecHandle();
}

void FPHPassiveResourceEffectRuntimeState::Reset()
{
	ResetSpecs();
	ResetActiveEffectHandles();
}

const TArray<FActiveGameplayEffectHandle>& FPHPassiveResourceEffectRuntimeState::GetActiveEffectHandles() const
{
	return ActiveEffectHandles;
}

void FPHAbilityInputRuntimeState::AddPressedSpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		PressedSpecHandles.AddUnique(Handle);
	}
}

void FPHAbilityInputRuntimeState::AddReleasedSpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		ReleasedSpecHandles.AddUnique(Handle);
	}
}

void FPHAbilityInputRuntimeState::AddHeldSpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		HeldSpecHandles.AddUnique(Handle);
	}
}

void FPHAbilityInputRuntimeState::RemoveHeldSpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	HeldSpecHandles.Remove(Handle);
}

void FPHAbilityInputRuntimeState::ResetFrameInput()
{
	PressedSpecHandles.Reset();
	ReleasedSpecHandles.Reset();
}

void FPHAbilityInputRuntimeState::Reset()
{
	ResetFrameInput();
	HeldSpecHandles.Reset();
}

const TArray<FGameplayAbilitySpecHandle>& FPHAbilityInputRuntimeState::GetPressedSpecHandles() const
{
	return PressedSpecHandles;
}

const TArray<FGameplayAbilitySpecHandle>& FPHAbilityInputRuntimeState::GetReleasedSpecHandles() const
{
	return ReleasedSpecHandles;
}

const TArray<FGameplayAbilitySpecHandle>& FPHAbilityInputRuntimeState::GetHeldSpecHandles() const
{
	return HeldSpecHandles;
}

void FPHStaminaDegenRequestRuntimeState::SetSprintRequested(const bool bRequested)
{
	bSprintRequested = bRequested;
}

void FPHStaminaDegenRequestRuntimeState::SetWallRunningRequested(const bool bRequested)
{
	bWallRunningRequested = bRequested;
}

void FPHStaminaDegenRequestRuntimeState::Clear()
{
	bSprintRequested = false;
	bWallRunningRequested = false;
}

bool FPHStaminaDegenRequestRuntimeState::IsSprintRequested() const
{
	return bSprintRequested;
}

bool FPHStaminaDegenRequestRuntimeState::IsWallRunningRequested() const
{
	return bWallRunningRequested;
}

bool FPHStaminaDegenRequestRuntimeState::HasAnyRequest() const
{
	return bSprintRequested || bWallRunningRequested;
}
