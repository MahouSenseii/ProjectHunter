// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/HUD/HunterHUD.h"

#include "Interactable/Widget/ItemTooltipWidget.h"
#include "Character/HUD/HunterHUD_HealthWidget.h"
#include "Character/HUD/HunterHUD_StaminaWidget.h"
#include "Character/HUD/HunterHUD_ManaWidget.h"
#include "Character/HUD/HunterHUD_XPWidget.h"
#include "Character/HUD/StatusEffect/StatusEffectHUDWidget.h"
#include "Character/PHBaseCharacter.h"
#include "GameFramework/PlayerController.h"

// ─────────────────────────────────────────────────────────────────────────────
// AHUD overrides
// ─────────────────────────────────────────────────────────────────────────────

void AHunterHUD::BeginPlay()
{
	Super::BeginPlay();

	// ── Tooltip widget ────────────────────────────────────────────────────
	if (ItemTooltipWidgetClass && GetWorld())
	{
		ItemTooltipWidget = CreateWidget<UItemTooltipWidget>(GetWorld(), ItemTooltipWidgetClass);
		if (ItemTooltipWidget)
		{
			ItemTooltipWidget->AddToViewport(100);  // High Z-order — always on top
			ItemTooltipWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// ── Stat widgets ──────────────────────────────────────────────────────
	CreateStatWidgets();

	// ── Pawn-change delegate: handles respawn / repossession ──────────────
	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->OnPossessedPawnChanged.AddDynamic(this, &AHunterHUD::HandlePawnChanged);

		// If the pawn is already possessed at BeginPlay (e.g. instant-spawn game mode),
		// bind immediately without waiting for the first delegate fire.
		if (APHBaseCharacter* Character = Cast<APHBaseCharacter>(PC->GetPawn()))
		{
			BindWidgetsToCharacter(Character);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Tooltip management
// ─────────────────────────────────────────────────────────────────────────────

void AHunterHUD::ShowItemTooltip(UItemInstance* Item, FVector2D ScreenPosition)
{
	if (ItemTooltipWidget && Item)
	{
		ItemTooltipWidget->UpdateTooltip(Item);
		ItemTooltipWidget->SetPositionInViewport(ScreenPosition);
		ItemTooltipWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void AHunterHUD::HideItemTooltip()
{
	if (ItemTooltipWidget)
	{
		ItemTooltipWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void AHunterHUD::ShowMashProgressWidget(const FText& Text, int32 INT32)
{
}

void AHunterHUD::HideMashProgressWidget()
{
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void AHunterHUD::CreateStatWidgets()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	// UI-1 FIX: Warn if any essential widget class is unset in Blueprint defaults.
	if (!HealthWidgetClass)  { UE_LOG(LogTemp, Warning, TEXT("HunterHUD: HealthWidgetClass not set in Blueprint")); }
	if (!StaminaWidgetClass) { UE_LOG(LogTemp, Warning, TEXT("HunterHUD: StaminaWidgetClass not set in Blueprint")); }
	if (!ManaWidgetClass)    { UE_LOG(LogTemp, Warning, TEXT("HunterHUD: ManaWidgetClass not set in Blueprint")); }
	if (!XPWidgetClass)      { UE_LOG(LogTemp, Warning, TEXT("HunterHUD: XPWidgetClass not set in Blueprint")); }

	// Create each widget (if the class was assigned in Blueprint defaults) and
	// add it to the viewport at Z-order 10 — above the world, below the tooltip.

	if (HealthWidgetClass)
	{
		HealthWidget = CreateWidget<UHunterHUD_HealthWidget>(PC, HealthWidgetClass);
		if (HealthWidget) { HealthWidget->AddToViewport(10); }
	}

	if (StaminaWidgetClass)
	{
		StaminaWidget = CreateWidget<UHunterHUD_StaminaWidget>(PC, StaminaWidgetClass);
		if (StaminaWidget) { StaminaWidget->AddToViewport(10); }
	}

	if (ManaWidgetClass)
	{
		ManaWidget = CreateWidget<UHunterHUD_ManaWidget>(PC, ManaWidgetClass);
		if (ManaWidget) { ManaWidget->AddToViewport(10); }
	}

	if (XPWidgetClass)
	{
		XPWidget = CreateWidget<UHunterHUD_XPWidget>(PC, XPWidgetClass);
		if (XPWidget) { XPWidget->AddToViewport(10); }
	}

	if (StatusEffectWidgetClass)
	{
		StatusEffectWidget = CreateWidget<UStatusEffectHUDWidget>(PC, StatusEffectWidgetClass);
		if (StatusEffectWidget) { StatusEffectWidget->AddToViewport(10); }
	}
}

void AHunterHUD::BindWidgetsToCharacter(APHBaseCharacter* Character)
{
	if (!Character)
	{
		// Pawn was unpossessed (death / menu) — release all bindings cleanly.
		if (HealthWidget)       { HealthWidget->ReleaseCharacter();       }
		if (StaminaWidget)      { StaminaWidget->ReleaseCharacter();      }
		if (ManaWidget)         { ManaWidget->ReleaseCharacter();         }
		if (XPWidget)           { XPWidget->ReleaseCharacter();           }
		if (StatusEffectWidget) { StatusEffectWidget->ReleaseCharacter(); }
		return;
	}

	// InitializeForCharacter safely releases any previous binding before rebinding,
	// so it is safe to call unconditionally on respawn.
	if (HealthWidget)        { HealthWidget->InitializeForCharacter(Character);        }
	if (StaminaWidget)       { StaminaWidget->InitializeForCharacter(Character);       }
	if (ManaWidget)          { ManaWidget->InitializeForCharacter(Character);          }
	if (XPWidget)            { XPWidget->InitializeForCharacter(Character);            }
	if (StatusEffectWidget)  { StatusEffectWidget->InitializeForCharacter(Character);  }
}

void AHunterHUD::HandlePawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	// NewPawn is null when the player is unpossessed (death, going to menu, etc.).
	APHBaseCharacter* NewCharacter = Cast<APHBaseCharacter>(NewPawn);
	BindWidgetsToCharacter(NewCharacter);
}
