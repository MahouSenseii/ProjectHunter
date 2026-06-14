// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"

/**
 * Single source of truth for the project's interaction trace channel.
 *
 * DefaultEngine.ini maps ECC_GameTraceChannel1 to the "Interactable" channel.
 * Every system that traces for interactables or configures collision responses
 * for them should reference THIS constant instead of a raw ECC_ value — change
 * the channel here once and everything stays in sync.
 *
 * NOTE: FInteractionTraceManager::InteractionTraceChannel currently defaults to
 * ECC_Visibility (interactables happen to block Visibility too, so focus works).
 * Once every interactable Blueprint blocks the dedicated channel, flip that
 * default to PHInteractionChannels::Interaction for precise interactable-only
 * traces (no more focusing random walls/props that merely block Visibility).
 */
namespace PHInteractionChannels
{
	inline constexpr ECollisionChannel Interaction = ECC_GameTraceChannel1;
}
