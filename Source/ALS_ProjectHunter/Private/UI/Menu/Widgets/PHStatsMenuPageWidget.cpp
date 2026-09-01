// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "UI/Menu/Widgets/PHStatsMenuPageWidget.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Character/PHBaseCharacter.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Progression/Components/CharacterProgressionManager.h"
#include "Tags/PHGameplayTags.h"
#include "UI/Menu/Helpers/MenuRowBuilder.h"

namespace
{
	using namespace PHMenuRowBuilder;

	/** Missing tags return empty rather than warning; the row is then skipped. */
	FGameplayTag Tag(const TCHAR* Name)
	{
		return FGameplayTag::RequestGameplayTag(FName(Name), /*ErrorIfNotFound*/ false);
	}

	FPHStatRowDef Row(const TCHAR* TagName, const TCHAR* Label,
		const EPHStatFormat Format = EPHStatFormat::SF_Integer,
		const FName Spendable = NAME_None)
	{
		FPHStatRowDef Def;
		Def.AttributeTag = Tag(TagName);
		Def.DisplayName = FText::FromString(Label);
		Def.Format = Format;
		Def.SpendableAttributeName = Spendable;
		return Def;
	}
}

UPHStatsMenuPageWidget::UPHStatsMenuPageWidget()
{
	BuildDefaultGroups();
}

void UPHStatsMenuPageWidget::BuildDefaultGroups()
{
	// Curated rather than exhaustive: the registry holds ~374 secondary
	// attributes, and a page that lists all of them is a wall of numbers.
	// These are the ones that describe a build. Adding more is data, not code.
	FPHStatGroupDef Primary;
	Primary.Title = FText::FromString(TEXT("ATTRIBUTES"));
	Primary.Rows = {
		Row(TEXT("Attributes.Primary.Strength"),     TEXT("Strength"),     EPHStatFormat::SF_Integer, TEXT("Strength")),
		Row(TEXT("Attributes.Primary.Dexterity"),    TEXT("Dexterity"),    EPHStatFormat::SF_Integer, TEXT("Dexterity")),
		Row(TEXT("Attributes.Primary.Intelligence"), TEXT("Intelligence"), EPHStatFormat::SF_Integer, TEXT("Intelligence")),
		Row(TEXT("Attributes.Primary.Endurance"),    TEXT("Endurance"),    EPHStatFormat::SF_Integer, TEXT("Endurance")),
		Row(TEXT("Attributes.Primary.Affliction"),   TEXT("Affliction"),   EPHStatFormat::SF_Integer, TEXT("Affliction")),
		Row(TEXT("Attributes.Primary.Luck"),         TEXT("Luck"),         EPHStatFormat::SF_Integer, TEXT("Luck")),
		Row(TEXT("Attributes.Primary.Covenant"),     TEXT("Covenant"),     EPHStatFormat::SF_Integer, TEXT("Covenant")),
	};

	FPHStatGroupDef Vitals;
	Vitals.Title = FText::FromString(TEXT("VITALS"));
	Vitals.Rows = {
		Row(TEXT("Attributes.Secondary.Vital.MaxHealth"),        TEXT("Maximum Health")),
		Row(TEXT("Attributes.Secondary.Vital.MaxMana"),          TEXT("Maximum Mana")),
		Row(TEXT("Attributes.Secondary.Vital.MaxStamina"),       TEXT("Maximum Stamina")),
		Row(TEXT("Attributes.Secondary.Vital.MaxArcaneShield"),  TEXT("Maximum Arcane Shield")),
		Row(TEXT("Attributes.Secondary.Vital.HealthRegenRate"),  TEXT("Health Regeneration"), EPHStatFormat::SF_Decimal),
		Row(TEXT("Attributes.Secondary.Vital.ManaRegenRate"),    TEXT("Mana Regeneration"),   EPHStatFormat::SF_Decimal),
		Row(TEXT("Attributes.Secondary.Vital.StaminaRegenRate"), TEXT("Stamina Regeneration"), EPHStatFormat::SF_Decimal),
	};

	FPHStatGroupDef Offence;
	Offence.Title = FText::FromString(TEXT("OFFENCE"));
	Offence.Rows = {
		Row(TEXT("Attributes.Secondary.Offensive.AttackSpeed"),         TEXT("Attack Speed"),        EPHStatFormat::SF_Decimal),
		Row(TEXT("Attributes.Secondary.Offensive.CastSpeed"),           TEXT("Cast Speed"),          EPHStatFormat::SF_Decimal),
		Row(TEXT("Attributes.Secondary.Offensive.CritChance"),          TEXT("Critical Chance"),     EPHStatFormat::SF_Percent),
		Row(TEXT("Attributes.Secondary.Offensive.CritMultiplier"),      TEXT("Critical Multiplier"), EPHStatFormat::SF_Decimal),
		Row(TEXT("Attributes.Secondary.Offensive.MeleeDamage"),         TEXT("Melee Damage")),
		Row(TEXT("Attributes.Secondary.Offensive.RangedDamage"),        TEXT("Ranged Damage")),
		Row(TEXT("Attributes.Secondary.Offensive.SpellDamage"),         TEXT("Spell Damage")),
		Row(TEXT("Attributes.Secondary.Offensive.ElementalDamage"),     TEXT("Elemental Damage")),
		Row(TEXT("Attributes.Secondary.Offensive.AreaOfEffect"),        TEXT("Area of Effect"),      EPHStatFormat::SF_Percent),
		Row(TEXT("Attributes.Secondary.Offensive.AttackRange"),         TEXT("Attack Range")),
	};

	FPHStatGroupDef Defence;
	Defence.Title = FText::FromString(TEXT("DEFENCE"));
	Defence.bStartExpanded = false;
	Defence.Rows = {
		Row(TEXT("Attributes.Secondary.Resistance.Armour"),         TEXT("Armour")),
		Row(TEXT("Attributes.Secondary.Resistance.GlobalDefenses"), TEXT("Global Defence"), EPHStatFormat::SF_Percent),
		Row(TEXT("Attributes.Secondary.Resistance.BlockStrength"),  TEXT("Block Strength"), EPHStatFormat::SF_Percent),
		Row(TEXT("Attributes.Secondary.Misc.Poise"),                TEXT("Poise")),
		Row(TEXT("Attributes.Secondary.Misc.StunRecovery"),         TEXT("Stun Recovery"),  EPHStatFormat::SF_Percent),
	};

	FPHStatGroupDef Resistances;
	Resistances.Title = FText::FromString(TEXT("RESISTANCES"));
	Resistances.bStartExpanded = false;
	Resistances.Rows = {
		Row(TEXT("Attributes.Secondary.Resistance.Fire.Percent"),       TEXT("Fire"),       EPHStatFormat::SF_Percent),
		Row(TEXT("Attributes.Secondary.Resistance.Ice.Percent"),        TEXT("Ice"),        EPHStatFormat::SF_Percent),
		Row(TEXT("Attributes.Secondary.Resistance.Lightning.Percent"),  TEXT("Lightning"),  EPHStatFormat::SF_Percent),
		Row(TEXT("Attributes.Secondary.Resistance.Light.Percent"),      TEXT("Light"),      EPHStatFormat::SF_Percent),
		Row(TEXT("Attributes.Secondary.Resistance.Corruption.Percent"), TEXT("Corruption"), EPHStatFormat::SF_Percent),
	};

	FPHStatGroupDef Utility;
	Utility.Title = FText::FromString(TEXT("UTILITY"));
	Utility.bStartExpanded = false;
	Utility.Rows = {
		Row(TEXT("Attributes.Secondary.Misc.MovementSpeed"), TEXT("Movement Speed"), EPHStatFormat::SF_Percent),
		Row(TEXT("Attributes.Secondary.Misc.LifeLeech"),     TEXT("Life Leech"),     EPHStatFormat::SF_Percent),
		Row(TEXT("Attributes.Secondary.Misc.ManaLeech"),     TEXT("Mana Leech"),     EPHStatFormat::SF_Percent),
		Row(TEXT("Attributes.Secondary.Misc.LifeOnHit"),     TEXT("Life on Hit")),
		Row(TEXT("Attributes.Secondary.Misc.ManaOnHit"),     TEXT("Mana on Hit")),
		Row(TEXT("Attributes.Secondary.Misc.CoolDown"),      TEXT("Cooldown Recovery"), EPHStatFormat::SF_Percent),
		Row(TEXT("Attributes.Secondary.Misc.Weight"),        TEXT("Carry Weight"),      EPHStatFormat::SF_Decimal),
	};

	StatGroups = {Primary, Vitals, Offence, Defence, Resistances, Utility};
}

TSharedRef<SWidget> UPHStatsMenuPageWidget::RebuildWidget()
{
	BuildWidgets();
	return Super::RebuildWidget();
}

void UPHStatsMenuPageWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshStats();
}

void UPHStatsMenuPageWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	Super::NativeInitializeForCharacter(Character);
	BuildWidgets();
	BindProgressionDelegates();
	RefreshStats();
}

void UPHStatsMenuPageWidget::NativeReleaseCharacter()
{
	UnbindProgressionDelegates();
	Super::NativeReleaseCharacter();
}

UCharacterProgressionManager* UPHStatsMenuPageWidget::GetProgression() const
{
	const APHBaseCharacter* Character = GetBoundCharacter();
	return Character ? Character->GetProgressionManager() : nullptr;
}

void UPHStatsMenuPageWidget::BindProgressionDelegates()
{
	if (UCharacterProgressionManager* Progression = GetProgression())
	{
		Progression->OnLevelUp.AddUniqueDynamic(this, &UPHStatsMenuPageWidget::HandleLevelUp);
		Progression->OnStatPointSpent.AddUniqueDynamic(this, &UPHStatsMenuPageWidget::HandleStatPointSpent);
		Progression->OnXPGained.AddUniqueDynamic(this, &UPHStatsMenuPageWidget::HandleXPGained);
	}
}

void UPHStatsMenuPageWidget::UnbindProgressionDelegates()
{
	if (UCharacterProgressionManager* Progression = GetProgression())
	{
		Progression->OnLevelUp.RemoveDynamic(this, &UPHStatsMenuPageWidget::HandleLevelUp);
		Progression->OnStatPointSpent.RemoveDynamic(this, &UPHStatsMenuPageWidget::HandleStatPointSpent);
		Progression->OnXPGained.RemoveDynamic(this, &UPHStatsMenuPageWidget::HandleXPGained);
	}
}

void UPHStatsMenuPageWidget::HandleLevelUp(int32, int32, int32) { RefreshStats(); }
void UPHStatsMenuPageWidget::HandleStatPointSpent(FName, int32) { RefreshStats(); }
void UPHStatsMenuPageWidget::HandleXPGained(int64, int64, float) { RefreshHeader(); }

void UPHStatsMenuPageWidget::BuildWidgets()
{
	if (bWidgetsBuilt || !WidgetTree)
	{
		return;
	}

	// An authored Blueprint may provide StatsContainer; otherwise the helper
	// builds a scrolling column, so this works as a plain C++ page class too.
	UPanelWidget* Host = EnsureRowHost(*WidgetTree, StatsContainer);
	if (!Host)
	{
		return;
	}

	BuiltColumn = AddPaddedColumn(*WidgetTree, *Host);

	// Header: level, experience, and what is left to spend.
	LevelText = MakeText(*WidgetTree, FText::GetEmpty(), 22, Palette::Text, TEXT("Bold"), 60.0f);
	AddRow(*BuiltColumn, *LevelText, 0.0f, 2.0f);

	ExperienceText = MakeText(*WidgetTree, FText::GetEmpty(), 12, Palette::Dim, TEXT("Regular"));
	AddRow(*BuiltColumn, *ExperienceText, 0.0f, 2.0f);

	PointsText = MakeText(*WidgetTree, FText::GetEmpty(), 13, Palette::Accent);
	AddRow(*BuiltColumn, *PointsText, 0.0f, 12.0f);

	GroupWidgets.Reset();
	HeaderButtonToGroup.Reset();
	SpendButtonToAttribute.Reset();

	for (int32 GroupIndex = 0; GroupIndex < StatGroups.Num(); ++GroupIndex)
	{
		const FPHStatGroupDef& Group = StatGroups[GroupIndex];
		FStatGroupWidgets Built;
		Built.bExpanded = Group.bStartExpanded;

		UTextBlock* Caret = nullptr;
		UButton* Header = MakeSectionHeader(*WidgetTree, Group.Title, Caret);
		Header->OnClicked.AddUniqueDynamic(this, &UPHStatsMenuPageWidget::HandleSectionToggled);
		HeaderButtonToGroup.Add(Header, GroupIndex);
		Built.Caret = Caret;
		AddRow(*BuiltColumn, *Header, 8.0f, 2.0f);

		UVerticalBox* RowBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Built.RowBox = RowBox;
		AddRow(*BuiltColumn, *RowBox, 0.0f, 4.0f);

		for (const FPHStatRowDef& Definition : Group.Rows)
		{
			// A tag that resolves to no attribute is skipped outright. Showing
			// it as 0 would be indistinguishable from a real value.
			if (!Definition.AttributeTag.IsValid())
			{
				continue;
			}

			FStatRowWidgets RowState;
			RowState.Tag = Definition.AttributeTag;
			RowState.Format = Definition.Format;
			RowState.SpendableAttributeName = Definition.SpendableAttributeName;

			UTextBlock* ValueText = nullptr;
			UHorizontalBox* RowBoxWidget = MakeStatRow(*WidgetTree,
				LabelForTag(Definition.AttributeTag, Definition.DisplayName), ValueText);
			RowState.ValueText = ValueText;

			if (!Definition.SpendableAttributeName.IsNone())
			{
				UButton* Spend = MakeButton(*WidgetTree, FText::FromString(TEXT("+")), 14);
				Spend->OnClicked.AddUniqueDynamic(this, &UPHStatsMenuPageWidget::HandleSpendClicked);
				SpendButtonToAttribute.Add(Spend, Definition.SpendableAttributeName);
				RowState.SpendButton = Spend;

				if (UHorizontalBoxSlot* SpendSlot = Cast<UHorizontalBoxSlot>(RowBoxWidget->AddChild(Spend)))
				{
					SpendSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
					SpendSlot->SetVerticalAlignment(VAlign_Center);
				}
			}

			AddRow(*RowBox, *RowBoxWidget, 2.0f, 2.0f);
			Built.Rows.Add(RowState);
		}

		GroupWidgets.Add(Built);
		ApplyGroupExpansion(GroupIndex);
	}

	bWidgetsBuilt = true;
}

bool UPHStatsMenuPageWidget::TryReadAttribute(const FGameplayTag& Tag, float& OutValue) const
{
	const APHBaseCharacter* Character = GetBoundCharacter();
	if (!Character)
	{
		return false;
	}
	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	const FGameplayAttribute Attribute = FPHGameplayTags::GetAttributeFromTag(Tag);
	if (!Attribute.IsValid())
	{
		return false;
	}

	bool bFound = false;
	OutValue = ASC->GetGameplayAttributeValue(Attribute, bFound);
	return bFound;
}

void UPHStatsMenuPageWidget::RefreshStats()
{
	RefreshHeader();

	const int32 Unspent = GetUnspentStatPoints();

	for (FStatGroupWidgets& Group : GroupWidgets)
	{
		for (FStatRowWidgets& RowState : Group.Rows)
		{
			if (UTextBlock* ValueText = RowState.ValueText.Get())
			{
				float Value = 0.0f;
				ValueText->SetText(TryReadAttribute(RowState.Tag, Value)
					? FormatValue(Value, RowState.Format)
					: FText::FromString(TEXT("--")));
			}

			// The spend control is hidden rather than disabled when there is
			// nothing to spend, so the page is quiet at zero points.
			if (UButton* Spend = RowState.SpendButton.Get())
			{
				Spend->SetVisibility(Unspent > 0
					? ESlateVisibility::Visible
					: ESlateVisibility::Collapsed);
			}
		}
	}

	OnStatsRefreshed();
}

void UPHStatsMenuPageWidget::RefreshHeader()
{
	const UCharacterProgressionManager* Progression = GetProgression();

	if (LevelText)
	{
		LevelText->SetText(Progression
			? FText::FromString(FString::Printf(TEXT("LEVEL %d"), Progression->Level))
			: FText::FromString(TEXT("LEVEL --")));
	}

	if (ExperienceText)
	{
		ExperienceText->SetText(Progression
			? FText::FromString(FString::Printf(TEXT("EXPERIENCE  %.0f%%"),
				Progression->GetXPProgressPercent() * 100.0f))
			: FText::GetEmpty());
	}

	if (PointsText)
	{
		if (Progression)
		{
			const int32 Stat = Progression->UnspentStatPoints;
			const int32 Passive = Progression->UnspentPassivePoints;
			PointsText->SetText(FText::FromString(FString::Printf(
				TEXT("%d STAT POINTS   /   %d PASSIVE POINTS"), Stat, Passive)));
			PointsText->SetVisibility((Stat > 0 || Passive > 0)
				? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
		else
		{
			PointsText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

int32 UPHStatsMenuPageWidget::GetUnspentStatPoints() const
{
	const UCharacterProgressionManager* Progression = GetProgression();
	return Progression ? Progression->UnspentStatPoints : 0;
}

bool UPHStatsMenuPageWidget::RequestSpendStatPoint(const FName AttributeName)
{
	UCharacterProgressionManager* Progression = GetProgression();
	if (!Progression || AttributeName.IsNone())
	{
		return false;
	}

	// The owner decides. It refuses without authority, without points, or when
	// no stat-point effect is configured for the attribute.
	const bool bSpent = Progression->SpendStatPoint(AttributeName);
	if (bSpent)
	{
		RefreshStats();
	}
	return bSpent;
}

void UPHStatsMenuPageWidget::HandleSectionToggled()
{
	// UButton::OnClicked carries no sender, so the pressed header is found by
	// asking each known header whether it is the one under the cursor.
	for (const TPair<TWeakObjectPtr<UButton>, int32>& Pair : HeaderButtonToGroup)
	{
		UButton* Button = Pair.Key.Get();
		if (Button && Button->IsHovered())
		{
			const int32 GroupIndex = Pair.Value;
			if (GroupWidgets.IsValidIndex(GroupIndex))
			{
				GroupWidgets[GroupIndex].bExpanded = !GroupWidgets[GroupIndex].bExpanded;
				ApplyGroupExpansion(GroupIndex);
			}
			return;
		}
	}
}

void UPHStatsMenuPageWidget::HandleSpendClicked()
{
	for (const TPair<TWeakObjectPtr<UButton>, FName>& Pair : SpendButtonToAttribute)
	{
		const UButton* Button = Pair.Key.Get();
		if (Button && Button->IsHovered())
		{
			RequestSpendStatPoint(Pair.Value);
			return;
		}
	}
}

void UPHStatsMenuPageWidget::ApplyGroupExpansion(const int32 GroupIndex)
{
	if (!GroupWidgets.IsValidIndex(GroupIndex))
	{
		return;
	}
	const FStatGroupWidgets& Group = GroupWidgets[GroupIndex];

	if (UVerticalBox* RowBox = Group.RowBox.Get())
	{
		RowBox->SetVisibility(Group.bExpanded
			? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (UTextBlock* Caret = Group.Caret.Get())
	{
		Caret->SetText(FText::FromString(Group.bExpanded ? TEXT("-") : TEXT("+")));
	}
}

FText UPHStatsMenuPageWidget::FormatValue(const float Value, const EPHStatFormat Format)
{
	switch (Format)
	{
	case EPHStatFormat::SF_Percent:
		return FText::FromString(FString::Printf(TEXT("%.1f%%"), Value));
	case EPHStatFormat::SF_Decimal:
		return FText::FromString(FString::Printf(TEXT("%.2f"), Value));
	case EPHStatFormat::SF_Seconds:
		return FText::FromString(FString::Printf(TEXT("%.2fs"), Value));
	default:
		return FText::AsNumber(FMath::RoundToInt(Value));
	}
}

FText UPHStatsMenuPageWidget::LabelForTag(const FGameplayTag& Tag, const FText& Explicit)
{
	if (!Explicit.IsEmpty())
	{
		return Explicit;
	}

	FString Name = Tag.GetTagName().ToString();
	int32 LastDot = INDEX_NONE;
	if (Name.FindLastChar(TEXT('.'), LastDot))
	{
		Name = Name.RightChop(LastDot + 1);
	}
	return FText::FromString(FName::NameToDisplayString(Name, /*bIsBool*/ false));
}
