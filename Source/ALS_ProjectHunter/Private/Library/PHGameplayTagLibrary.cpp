// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/PHGameplayTagLibrary.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

static UAbilitySystemComponent* GetASC(const AActor* Actor)
{
	return Actor ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor) : nullptr;
}

bool UPHGameplayTagLibrary::CheckTag(const AActor* Actor, FGameplayTag Tag, bool bExactMatch)
{
	if (!Actor || !Tag.IsValid()) return false;

	if (UAbilitySystemComponent* ASC = GetASC(Actor))
	{
		// Pull the owned tags and use exact/non-exact helpers
		FGameplayTagContainer Owned;
		ASC->GetOwnedGameplayTags(Owned);
		return bExactMatch ? Owned.HasTagExact(Tag) : Owned.HasTag(Tag);
	}
	return false;
}

bool UPHGameplayTagLibrary::CheckTagByName(const AActor* Actor, FName TagName, bool bExactMatch)
{
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, /*ErrorIfNotFound*/ false);
	return Tag.IsValid() && CheckTag(Actor, Tag, bExactMatch);
}

bool UPHGameplayTagLibrary::CheckTagsAny(const AActor* Actor, const FGameplayTagContainer& Tags, bool bExactMatch)
{
	if (!Actor || Tags.IsEmpty()) return false;

	if (UAbilitySystemComponent* ASC = GetASC(Actor))
	{
		FGameplayTagContainer Owned;
		ASC->GetOwnedGameplayTags(Owned);
		return bExactMatch ? Owned.HasAnyExact(Tags) : Owned.HasAny(Tags);
	}
	return false;
}

bool UPHGameplayTagLibrary::CheckTagsAll(const AActor* Actor, const FGameplayTagContainer& Tags, bool bExactMatch)
{
	if (!Actor || Tags.IsEmpty()) return false;

	if (UAbilitySystemComponent* ASC = GetASC(Actor))
	{
		FGameplayTagContainer Owned;
		ASC->GetOwnedGameplayTags(Owned);
		return bExactMatch ? Owned.HasAllExact(Tags) : Owned.HasAll(Tags);
	}
	return false;
}

bool UPHGameplayTagLibrary::CheckActorNameTag(const AActor* Actor, FName NameTag)
{
	return (Actor && !NameTag.IsNone()) ? Actor->ActorHasTag(NameTag) : false;
}