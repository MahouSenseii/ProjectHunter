// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/PassiveManager.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "Character/PHBaseCharacter.h"
#include "Library/PHCharacterStructLibrary.h"
#include "Library/PHItemStructLibrary.h"


UPassiveManager::UPassiveManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPassiveManager::BeginPlay()
{
	Super::BeginPlay();

	if (const APHBaseCharacter* OwnerChar = Cast<APHBaseCharacter>(GetOwner()))
	{
		ASC = OwnerChar->GetAbilitySystemComponent();
	}
}

void UPassiveManager::ApplyPassive(const FPassiveEffectInfo& Passive)
{
	if (!ASC || !Passive.PassiveEffect || Passive.bUnlocked == false) return;

	FPassiveHandleList HandleList;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Passive.PassiveEffect, 1.0f, Context);
	if (Spec.IsValid())
	{
		FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		HandleList.Handles.Add(Handle);
	}

	// Store active handles for removal
	if (!HandleList.Handles.IsEmpty())
	{
		AppliedPassives.Add(Passive.PassiveID, HandleList);
		UnlockedPassiveIDs.Add(Passive.PassiveID);
	}
}

void UPassiveManager::ApplyPassiveGroup(const FPHCharacterPassive& PassiveGroup)
{
	for (const FPassiveEffectInfo& Effect : PassiveGroup.PassiveEffects)
	{
		ApplyPassive(Effect);
	}
}

void UPassiveManager::RemovePassiveByID(FName PassiveID)
{
	if (!ASC || !AppliedPassives.Contains(PassiveID)) return;

	const FPassiveHandleList& List = AppliedPassives[PassiveID];

	for (const FActiveGameplayEffectHandle& Handle : List.Handles)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	AppliedPassives.Remove(PassiveID);
	UnlockedPassiveIDs.Remove(PassiveID);
}

void UPassiveManager::RemoveAllPassives()
{
	TArray<FName> Keys;
	AppliedPassives.GetKeys(Keys);

	for (const FName& PassiveID : Keys)
	{
		RemovePassiveByID(PassiveID);
	}

	AppliedPassives.Empty();
	UnlockedPassiveIDs.Empty();
}

