#include "Character/HUD/StatusEffect/StatusEffectHUDWidget.h"
#include "Character/HUD/StatusEffect/StatusEffectIconWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Character/PHBaseCharacter.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

DEFINE_LOG_CATEGORY(LogStatusEffectHUD);

void UStatusEffectHUDWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Character);
	if (!ASCInterface)
	{
		UE_LOG(LogStatusEffectHUD, Warning,
			TEXT("StatusEffectHUDWidget: Character '%s' does not implement IAbilitySystemInterface"),
			*Character->GetName());
		return;
	}

	UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	OnEffectAddedHandle = ASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(
		this, &UStatusEffectHUDWidget::OnGameplayEffectAdded);

	OnEffectRemovedHandle = ASC->OnAnyGameplayEffectRemovedDelegate().AddUObject(
		this, &UStatusEffectHUDWidget::OnGameplayEffectRemoved);

	{
		const FGameplayEffectQuery Query =
			StatusEffectTagFilter.IsEmpty()
				? FGameplayEffectQuery()
				: FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(StatusEffectTagFilter);

		TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);
		for (const FActiveGameplayEffectHandle& Handle : Handles)
		{
			const FActiveGameplayEffect* AGE = ASC->GetActiveGameplayEffect(Handle);
			if (AGE)
			{
				AddIconForEffect(ASC, AGE->Handle, AGE->Spec);
			}
		}
	}

	UE_LOG(LogStatusEffectHUD, Log,
		TEXT("StatusEffectHUDWidget: Bound to character '%s' (%d initial effects)"),
		*Character->GetName(), ActiveIcons.Num());
}

void UStatusEffectHUDWidget::NativeReleaseCharacter()
{
	if (BoundCharacter.IsValid())
	{
		IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(BoundCharacter.Get());
		if (ASCInterface)
		{
			if (UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent())
			{
				ASC->OnActiveGameplayEffectAddedDelegateToSelf.Remove(OnEffectAddedHandle);
				ASC->OnAnyGameplayEffectRemovedDelegate().Remove(OnEffectRemovedHandle);
			}
		}
	}

	OnEffectAddedHandle.Reset();
	OnEffectRemovedHandle.Reset();

	for (auto& Pair : ActiveIcons)
	{
		if (Pair.Value)
		{
			Pair.Value->UnbindEffect();
			Pair.Value->RemoveFromParent();
		}
	}
	ActiveIcons.Empty();
}

void UStatusEffectHUDWidget::RefreshAllIcons()
{
	if (!BoundCharacter.IsValid())
	{
		return;
	}

	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(BoundCharacter.Get());
	if (!ASCInterface)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	for (auto& Pair : ActiveIcons)
	{
		if (Pair.Value)
		{
			Pair.Value->UnbindEffect();
			Pair.Value->RemoveFromParent();
		}
	}
	ActiveIcons.Empty();

	{
		const FGameplayEffectQuery Query =
			StatusEffectTagFilter.IsEmpty()
				? FGameplayEffectQuery()
				: FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(StatusEffectTagFilter);

		TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);
		for (const FActiveGameplayEffectHandle& Handle : Handles)
		{
			const FActiveGameplayEffect* AGE = ASC->GetActiveGameplayEffect(Handle);
			if (AGE)
			{
				AddIconForEffect(ASC, AGE->Handle, AGE->Spec);
			}
		}
	}
}

void UStatusEffectHUDWidget::OnGameplayEffectAdded(UAbilitySystemComponent* ASC,
	const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	if (!ASC)
	{
		return;
	}

	FGameplayTagContainer GrantedTags;
	Spec.GetAllGrantedTags(GrantedTags);

	if (!PassesTagFilter(GrantedTags))
	{
		return;
	}

	AddIconForEffect(ASC, Handle, Spec);
}

void UStatusEffectHUDWidget::OnGameplayEffectRemoved(const FActiveGameplayEffect& Effect)
{
	RemoveIconForHandle(Effect.Handle);
}

void UStatusEffectHUDWidget::AddIconForEffect(UAbilitySystemComponent* ASC,
	FActiveGameplayEffectHandle Handle,
	const FGameplayEffectSpec& Spec)
{
	if (!IconWidgetClass)
	{
		UE_LOG(LogStatusEffectHUD, Warning,
			TEXT("StatusEffectHUDWidget: IconWidgetClass is not set"));
		return;
	}

	if (ActiveIcons.Contains(Handle))
	{
		return;
	}

	if (ActiveIcons.Num() >= MaxVisibleIcons)
	{
		UE_LOG(LogStatusEffectHUD, Verbose,
			TEXT("StatusEffectHUDWidget: MaxVisibleIcons (%d) reached — skipping new icon"),
			MaxVisibleIcons);
		return;
	}

	UHorizontalBox* Container = GetIconContainer();
	if (!Container)
	{
		return;
	}

	FGameplayTagContainer GrantedTags;
	Spec.GetAllGrantedTags(GrantedTags);

	UTexture2D* Icon = BP_GetIconForEffect(GrantedTags);
	const bool bIsBuff = BP_IsEffectBuff(GrantedTags);

	FText EffectName = FText::GetEmpty();
	if (Spec.Def != nullptr)
	{
		EffectName = FText::FromString(Spec.Def->GetName());
	}

	UStatusEffectIconWidget* IconWidget =
		CreateWidget<UStatusEffectIconWidget>(GetOwningPlayer(), IconWidgetClass);
	if (!IconWidget)
	{
		return;
	}

	IconWidget->BindToEffect(ASC, Handle, Icon, EffectName, bIsBuff);

	UHorizontalBoxSlot* HSlot = Container->AddChildToHorizontalBox(IconWidget);
	if (Slot)
	{
		HSlot->SetPadding(FMargin(4.0f, 0.0f));
		HSlot->SetVerticalAlignment(VAlign_Center);
	}

	ActiveIcons.Add(Handle, IconWidget);
	BP_OnIconAdded(IconWidget);

	UE_LOG(LogStatusEffectHUD, Verbose,
		TEXT("StatusEffectHUDWidget: Added icon for effect '%s' (%d active)"),
		*EffectName.ToString(), ActiveIcons.Num());
}

void UStatusEffectHUDWidget::RemoveIconForHandle(FActiveGameplayEffectHandle Handle)
{
	TObjectPtr<UStatusEffectIconWidget>* Found = ActiveIcons.Find(Handle);
	if (!Found || !(*Found))
	{
		return;
	}

	UStatusEffectIconWidget* IconWidget = Found->Get();
	IconWidget->UnbindEffect();
	IconWidget->RemoveFromParent();

	ActiveIcons.Remove(Handle);
	BP_OnIconRemoved(Handle);

	UE_LOG(LogStatusEffectHUD, Verbose,
		TEXT("StatusEffectHUDWidget: Removed icon (%d remaining)"), ActiveIcons.Num());
}

bool UStatusEffectHUDWidget::PassesTagFilter(const FGameplayTagContainer& GrantedTags) const
{
	if (StatusEffectTagFilter.IsEmpty())
	{
		return true;
	}
	return GrantedTags.HasAnyExact(StatusEffectTagFilter);
}

UHorizontalBox* UStatusEffectHUDWidget::GetIconContainer() const
{
	return Cast<UHorizontalBox>(GetWidgetFromName(IconContainerSlotName));
}
