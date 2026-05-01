// Tags/Debug/TagDebugManager.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TagDebugManager.generated.h"

class UTagManager;

DECLARE_LOG_CATEGORY_EXTERN(LogTagDebugManager, Log, All);

/**
 * On-screen tag state visualiser, mirroring the FStatsDebugManager pattern.
 *
 * Drop this as a UPROPERTY on UTagManager. When bEnableDebug is true the
 * component's Tick calls DrawDebug() every frame; tags are drawn as
 * persistent GEngine messages grouped by semantic category.  Only when the
 * owned tag container actually changes are the messages refreshed, so the
 * cost at steady-state is a single FGameplayTagContainer comparison.
 *
 * Message key range: [BaseMessageKey, BaseMessageKey + MaxLines).
 * Default BaseMessageKey = 52000 — safely above StatsDebugManager (50000).
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FTagDebugManager
{
    GENERATED_BODY()

    FTagDebugManager();

    // ── Master controls ────────────────────────────────────────────────────────

    /** Enable / disable the debug overlay. Toggle at runtime or in the Details panel. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug")
    bool bEnableDebug;

    /** Render tag state to the viewport as persistent on-screen messages. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Display")
    bool bDrawToScreen;

    /** Echo tag state changes to the output log. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Display")
    bool bLogToOutput;

    /**
     * First GEngine message key used by this panel.
     * Must not collide with StatsDebugManager (default 50000).
     * Reserve ~100 keys above this value.  Default: 52000.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Display", meta = (ClampMin = "0"))
    int32 BaseMessageKey;

    /**
     * When true, inactive (false) tags are shown dimmed alongside active ones
     * so you can see the full state at a glance.
     * When false, only tags that are currently ON are displayed.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Display")
    bool bShowInactiveTags;

    // ── Per-group visibility filters ───────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Groups")
    bool bShowLifeDeath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Groups")
    bool bShowThresholds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Groups")
    bool bShowMovement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Groups")
    bool bShowCombat;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Groups")
    bool bShowAilments;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Groups")
    bool bShowImmunities;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Groups")
    bool bShowEffects;

    /** Any GAS-granted tags not covered by the named groups above (abilities, buffs, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Groups")
    bool bShowOther;

    // ── Public API ─────────────────────────────────────────────────────────────

    /**
     * Main entry point — call from UTagManager::TickComponent.
     * Internally performs change detection; skips all work when tags are stable.
     */
    void DrawDebug(UTagManager* TagManager, UObject* WorldContext);

private:

    // ── Helpers ────────────────────────────────────────────────────────────────

    void BuildDisplayLines(UTagManager* TagManager,
                           const FGameplayTagContainer& OwnedTags,
                           TArray<FString>& OutLines,
                           TArray<FColor>& OutColors) const;

    /** Returns true when the owned tag container has changed since the last call. */
    bool CheckForTagChanges(UTagManager* TagManager);

    /** Remove all on-screen messages owned by this panel and reset line tracking. */
    void ClearDrawnMessages();

    /**
     * Strip well-known root prefixes ("Condition.", "Effect.") from a tag name
     * while preserving disambiguation ("Self.Stunned" vs "Target.Stunned").
     */
    static FString GetShortTagName(const FGameplayTag& Tag);

    // ── Runtime state (not UPROPERTY — debug-only, no GC concern) ─────────────

    bool bCacheInitialized;
    FGameplayTagContainer CachedActiveTags;
    TArray<FString>       CachedDisplayLines;
    TArray<FColor>        CachedLineColors;
    int32                 LastDrawnLineCount;
};
