// Tags/Debug/TagDebugManager.cpp

#include "Tags/Debug/TagDebugManager.h"
#include "Tags/Components/TagManager.h"
#include "Engine/Engine.h"
#include "PHGameplayTags.h"

DEFINE_LOG_CATEGORY(LogTagDebugManager);

// ─────────────────────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────────────────────

namespace TagDebugPrivate
{
    // On-screen message display duration — long enough to act as "persistent"
    // since we overwrite by key rather than letting them expire.
    constexpr float PersistentDuration = 3600.f;

    // ── Colour palette ────────────────────────────────────────────────────────

    // Category header rows
    const FColor HeaderColor(155, 155, 165);

    // Active tag colours per semantic group
    const FColor ActiveLifeDeathColor(230, 230, 50);   // yellow  — life/death states
    const FColor ActiveThresholdColor(100, 210, 255);  // cyan    — resource thresholds
    const FColor ActiveMovementColor(80,  200, 120);   // green   — movement/sprint
    const FColor ActiveCombatColor(255, 165, 50);      // orange  — combat states
    const FColor ActiveAilmentColor(220,  80,  80);    // red     — ailments/debuffs
    const FColor ActiveImmunityColor(200, 200, 80);    // gold    — immunities
    const FColor ActiveEffectColor(100, 180, 255);     // sky-blue— active GE effects
    const FColor ActiveOtherColor(200, 200, 200);      // white   — unknown/misc tags

    // Inactive (tag is OFF) — always the same dim grey regardless of group
    const FColor InactiveColor(55, 55, 55);

    // Panel title bar
    const FColor TitleColor(180, 180, 190);
}

// ─────────────────────────────────────────────────────────────────────────────
// FTagDebugManager — constructor / defaults
// ─────────────────────────────────────────────────────────────────────────────

FTagDebugManager::FTagDebugManager()
    : bEnableDebug(false)
    , bDrawToScreen(true)
    , bLogToOutput(false)
    , BaseMessageKey(52000)
    , bShowInactiveTags(true)
    , bShowLifeDeath(true)
    , bShowThresholds(true)
    , bShowMovement(true)
    , bShowCombat(true)
    , bShowAilments(true)
    , bShowImmunities(false)   // off by default — rarely interesting at runtime
    , bShowEffects(true)
    , bShowOther(true)
    , bCacheInitialized(false)
    , LastDrawnLineCount(0)
{
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

FString FTagDebugManager::GetShortTagName(const FGameplayTag& Tag)
{
    // Strip only the top-level root prefix ("Condition." / "Effect.") so that
    // "Self.Stunned" and "Target.Stunned" remain distinct in the display.
    const FString Name = Tag.GetTagName().ToString();

    static const TCHAR* const Prefixes[] = {
        TEXT("Condition."),
        TEXT("Effect.")
    };

    for (const TCHAR* Prefix : Prefixes)
    {
        const int32 PrefixLen = FCString::Strlen(Prefix);
        if (Name.StartsWith(Prefix, ESearchCase::CaseSensitive))
        {
            return Name.RightChop(PrefixLen);
        }
    }

    return Name;
}

// ─────────────────────────────────────────────────────────────────────────────

bool FTagDebugManager::CheckForTagChanges(UTagManager* TagManager)
{
    if (!TagManager)
    {
        return false;
    }

    FGameplayTagContainer CurrentTags;
    if (!TagManager->GetOwnedTags(CurrentTags))
    {
        // ASC not ready yet — treat as changed so the "no ASC" banner is redrawn.
        return !bCacheInitialized;
    }

    if (!bCacheInitialized || CurrentTags != CachedActiveTags)
    {
        CachedActiveTags   = CurrentTags;
        bCacheInitialized  = true;
        return true;
    }

    return false;
}

// ─────────────────────────────────────────────────────────────────────────────

void FTagDebugManager::ClearDrawnMessages()
{
    if (GEngine && LastDrawnLineCount > 0)
    {
        for (int32 i = 0; i < LastDrawnLineCount; ++i)
        {
            GEngine->RemoveOnScreenDebugMessage(static_cast<uint64>(BaseMessageKey + i));
        }
    }

    LastDrawnLineCount = 0;
    CachedDisplayLines.Reset();
    CachedLineColors.Reset();

    // Invalidate cache so the first re-enable always forces a full redraw.
    bCacheInitialized = false;
    CachedActiveTags.Reset();
}

// ─────────────────────────────────────────────────────────────────────────────
// BuildDisplayLines
// ─────────────────────────────────────────────────────────────────────────────

void FTagDebugManager::BuildDisplayLines(UTagManager* TagManager,
                                         const FGameplayTagContainer& OwnedTags,
                                         TArray<FString>& OutLines,
                                         TArray<FColor>& OutColors) const
{
    OutLines.Reset();
    OutColors.Reset();

    // ── Title bar ─────────────────────────────────────────────────────────────
    const FString OwnerName = (TagManager && TagManager->GetOwner())
                            ? TagManager->GetOwner()->GetName()
                            : TEXT("Unknown");

    OutLines.Add(FString::Printf(TEXT("=== TAG DEBUG : %s ==="), *OwnerName));
    OutColors.Add(TagDebugPrivate::TitleColor);

    if (!TagManager)
    {
        OutLines.Add(TEXT("  (no TagManager)"));
        OutColors.Add(FColor::Red);
        return;
    }

    // ── Tag group helper ──────────────────────────────────────────────────────
    //
    // Each logical group is described inline below. We accumulate a TSet of all
    // known tags as we go so that the "Other" group can exclude them at the end.

    TSet<FGameplayTag> KnownTags;

    // WriteGroup — draws a header + each tag in the group.
    // bGroupVisible: respect the per-group filter toggle.
    auto WriteGroup = [&](const TCHAR* Header,
                          bool bGroupVisible,
                          const FColor& ActiveColor,
                          std::initializer_list<FGameplayTag> GroupTags)
    {
        if (!bGroupVisible)
        {
            // Still register into KnownTags even when filtered out so "Other"
            // doesn't accidentally absorb them.
            for (const FGameplayTag& Tag : GroupTags)
            {
                if (Tag.IsValid()) { KnownTags.Add(Tag); }
            }
            return;
        }

        // Decide whether this group has anything to show before emitting the header.
        bool bHasContent = false;
        for (const FGameplayTag& Tag : GroupTags)
        {
            if (!Tag.IsValid()) { continue; }
            KnownTags.Add(Tag);
            if (bShowInactiveTags || OwnedTags.HasTagExact(Tag))
            {
                bHasContent = true;
            }
        }

        if (!bHasContent) { return; }

        OutLines.Add(FString::Printf(TEXT("-- %s --"), Header));
        OutColors.Add(TagDebugPrivate::HeaderColor);

        for (const FGameplayTag& Tag : GroupTags)
        {
            if (!Tag.IsValid()) { continue; }

            const bool bActive = OwnedTags.HasTagExact(Tag);
            if (!bActive && !bShowInactiveTags) { continue; }

            OutLines.Add(FString::Printf(TEXT("  %s %s"),
                bActive ? TEXT("[●]") : TEXT("[ ]"),
                *GetShortTagName(Tag)));
            OutColors.Add(bActive ? ActiveColor : TagDebugPrivate::InactiveColor);
        }
    };

    // ── Groups ────────────────────────────────────────────────────────────────

    const FPHGameplayTags& T = FPHGameplayTags::Get();

    // 1. Life / Death
    WriteGroup(TEXT("Life / Death"), bShowLifeDeath, TagDebugPrivate::ActiveLifeDeathColor,
    {
        T.Condition_Alive,
        T.Condition_Dead,
        T.Condition_NearDeathExperience,
        T.Condition_DeathPrevented,
    });

    // 2. Resource Thresholds (driven by attribute-change delegates)
    WriteGroup(TEXT("Resource Thresholds"), bShowThresholds, TagDebugPrivate::ActiveThresholdColor,
    {
        T.Condition_OnFullHealth,    T.Condition_OnLowHealth,
        T.Condition_OnFullMana,      T.Condition_OnLowMana,
        T.Condition_OnFullStamina,   T.Condition_OnLowStamina,
        T.Condition_OnFullArcaneShield, T.Condition_OnLowArcaneShield,
    });

    // 3. Movement / Sprint (polled at 100 ms cadence by TagManager)
    WriteGroup(TEXT("Movement"), bShowMovement, TagDebugPrivate::ActiveMovementColor,
    {
        T.Condition_WhileMoving,
        T.Condition_WhileStationary,
        T.Condition_Sprinting,
    });

    // 4. Combat — broad set covering action and interaction states
    WriteGroup(TEXT("Combat"), bShowCombat, TagDebugPrivate::ActiveCombatColor,
    {
        T.Condition_InCombat,
        T.Condition_OutOfCombat,
        T.Condition_TakingDamage,
        T.Condition_DealingDamage,
        T.Condition_RecentlyHit,
        T.Condition_RecentlyCrit,
        T.Condition_OnKill,
        T.Condition_OnCrit,
        T.Condition_RecentlyBlocked,
        T.Condition_RecentlyReflected,
        T.Condition_RecentlyUsedSkill,
        T.Condition_RecentlyAppliedBuff,
        T.Condition_RecentlyDispelled,
        T.Condition_UsingSkill,
        T.Condition_UsingMelee,
        T.Condition_UsingRanged,
        T.Condition_UsingSpell,
        T.Condition_UsingAura,
        T.Condition_UsingMovementSkill,
        T.Condition_WhileChanneling,
        T.Condition_Self_IsBlocking,
        T.Condition_TargetIsBoss,
        T.Condition_TargetIsMinion,
        T.Condition_TargetHasShield,
        T.Condition_TargetIsCasting,
    });

    // 5a. Ailments — Self
    WriteGroup(TEXT("Ailments (Self)"), bShowAilments, TagDebugPrivate::ActiveAilmentColor,
    {
        T.Condition_Self_Bleeding,
        T.Condition_Self_Stunned,
        T.Condition_Self_Frozen,
        T.Condition_Self_Shocked,
        T.Condition_Self_Burned,
        T.Condition_Self_Corrupted,
        T.Condition_Self_Purified,
        T.Condition_Self_Petrified,
        T.Condition_Self_CannotRegenHP,
        T.Condition_Self_CannotRegenStamina,
        T.Condition_Self_CannotRegenMana,
        T.Condition_Self_CannotHealHPAbove50Percent,
        T.Condition_Self_CannotHealStamina50Percent,
        T.Condition_Self_CannotHealMana50Percent,
        T.Condition_Self_LowArcaneShield,
        T.Condition_Self_ZeroArcaneShield,
    });

    // 5b. Ailments — Target
    WriteGroup(TEXT("Ailments (Target)"), bShowAilments, TagDebugPrivate::ActiveAilmentColor,
    {
        T.Condition_Target_Bleeding,
        T.Condition_Target_Stunned,
        T.Condition_Target_Frozen,
        T.Condition_Target_Shocked,
        T.Condition_Target_Burned,
        T.Condition_Target_Corrupted,
        T.Condition_Target_Purified,
        T.Condition_Target_Petrified,
        T.Condition_Target_IsBlocking,
    });

    // 6. Immunities
    WriteGroup(TEXT("Immunities"), bShowImmunities, TagDebugPrivate::ActiveImmunityColor,
    {
        T.Condition_ImmuneToCC,
        T.Condition_CannotBeFrozen,
        T.Condition_CannotBeCorrupted,
        T.Condition_CannotBeBurned,
        T.Condition_CannotBeSlowed,
        T.Condition_CannotBeInterrupted,
        T.Condition_CannotBeKnockedBack,
    });

    // 7. Active GE Effects
    WriteGroup(TEXT("Effects"), bShowEffects, TagDebugPrivate::ActiveEffectColor,
    {
        T.Effect_Health_RegenActive,
        T.Effect_Mana_RegenActive,
        T.Effect_Stamina_RegenActive,
        T.Effect_ArcaneShield_RegenActive,
        T.Effect_Health_DegenActive,
        T.Effect_Mana_DegenActive,
        T.Effect_Stamina_DegenActive,
    });

    // 8. Other — any tags NOT covered by the groups above (GE-granted, ability, etc.)
    if (bShowOther)
    {
        TArray<FGameplayTag> OtherTags;
        for (auto It = OwnedTags.CreateConstIterator(); It; ++It)
        {
            if (!KnownTags.Contains(*It))
            {
                OtherTags.Add(*It);
            }
        }

        if (OtherTags.Num() > 0)
        {
            OutLines.Add(TEXT("-- Other --"));
            OutColors.Add(TagDebugPrivate::HeaderColor);

            for (const FGameplayTag& Tag : OtherTags)
            {
                // "Other" tags are always active (they're in the owned container).
                OutLines.Add(FString::Printf(TEXT("  [●] %s"), *Tag.GetTagName().ToString()));
                OutColors.Add(TagDebugPrivate::ActiveOtherColor);
            }
        }
    }

    // Guarantee at least one line so the panel title is visible even when empty.
    if (OutLines.Num() == 1)
    {
        OutLines.Add(TEXT("  (no tags active)"));
        OutColors.Add(TagDebugPrivate::InactiveColor);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DrawDebug — main entry point (called from UTagManager::TickComponent)
// ─────────────────────────────────────────────────────────────────────────────

void FTagDebugManager::DrawDebug(UTagManager* TagManager, UObject* WorldContext)
{
    if (!bEnableDebug)
    {
        ClearDrawnMessages();
        return;
    }

    // ── Change detection ──────────────────────────────────────────────────────
    // All work is skipped when the tag container hasn't changed since last frame.
    const bool bTagsChanged = CheckForTagChanges(TagManager);

    // ── Screen draw ───────────────────────────────────────────────────────────
    if (bDrawToScreen && GEngine)
    {
        if (bTagsChanged || CachedDisplayLines.IsEmpty())
        {
            // Preserve old lines so we can skip re-submitting unchanged rows
            // (avoids the per-line flicker that occurs on every AddOnScreenDebugMessage call).
            TArray<FString> OldLines;
            Swap(OldLines, CachedDisplayLines);

            BuildDisplayLines(TagManager, CachedActiveTags, CachedDisplayLines, CachedLineColors);

            for (int32 i = 0; i < CachedDisplayLines.Num(); ++i)
            {
                const bool bLineChanged = !OldLines.IsValidIndex(i)
                                       || OldLines[i] != CachedDisplayLines[i];
                if (!bLineChanged) { continue; }

                const uint64 Key      = static_cast<uint64>(BaseMessageKey + i);
                const FColor LineColor = CachedLineColors.IsValidIndex(i)
                                      ? CachedLineColors[i]
                                      : FColor::White;

                GEngine->AddOnScreenDebugMessage(Key, TagDebugPrivate::PersistentDuration,
                                                 LineColor, CachedDisplayLines[i],
                                                 /*bNewerOnTop=*/false);
            }

            // Remove stale lines when the display shrinks (e.g. filter toggled).
            for (int32 i = CachedDisplayLines.Num(); i < LastDrawnLineCount; ++i)
            {
                GEngine->RemoveOnScreenDebugMessage(static_cast<uint64>(BaseMessageKey + i));
            }

            LastDrawnLineCount = CachedDisplayLines.Num();
        }
    }
    else if (!bDrawToScreen)
    {
        ClearDrawnMessages();
    }

    // ── Log output ────────────────────────────────────────────────────────────
    if (bLogToOutput && bTagsChanged)
    {
        const FString OwnerName = (TagManager && TagManager->GetOwner())
                                ? TagManager->GetOwner()->GetName()
                                : TEXT("Unknown");

        for (const FString& Line : CachedDisplayLines)
        {
            UE_LOG(LogTagDebugManager, Log, TEXT("[%s] %s"), *OwnerName, *Line);
        }
    }
}
