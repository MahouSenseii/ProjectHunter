#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "AffixPoolValidationLibrary.generated.h"

class UDataTable;

/** What a validation issue means. Errors break generation; warnings are almost always mistakes. */
UENUM(BlueprintType)
enum class EAffixPoolIssueSeverity : uint8
{
	APS_Info    UMETA(DisplayName = "Info"),
	APS_Warning UMETA(DisplayName = "Warning"),
	APS_Error   UMETA(DisplayName = "Error"),
};

UENUM(BlueprintType)
enum class EAffixPoolIssueType : uint8
{
	/** A pool lists an AffixID the definition tables do not contain. */
	API_MissingAffixDefinition  UMETA(DisplayName = "Missing Affix Definition"),

	/** An affix is defined but no sub-type pool reaches it, so it can never roll. */
	API_UnreachableAffix        UMETA(DisplayName = "Unreachable Affix"),

	/** A suffix listed under Prefixes, or the reverse. */
	API_WrongAffixType          UMETA(DisplayName = "Wrong Affix Type"),

	/** ForceTier names a tier the affix does not have. */
	API_ForceTierNotFound       UMETA(DisplayName = "Force Tier Not Found"),

	/** Two sets claim the same sub-type; only one is used. */
	API_DuplicateSubTypeSet     UMETA(DisplayName = "Duplicate Sub-Type Set"),

	/** An IncludedSets handle points at a row that does not exist. */
	API_MissingIncludeRow       UMETA(DisplayName = "Missing Include Row"),

	/** A set is reachable from its own includes. */
	API_IncludeCycle            UMETA(DisplayName = "Include Cycle"),

	/** A gear sub-type has no set, so it falls back to the whole shared table. */
	API_SubTypeWithoutPool      UMETA(DisplayName = "Sub-Type Without Pool"),

	/** A pool resolved to no prefixes and no suffixes. */
	API_EmptyPool               UMETA(DisplayName = "Empty Pool"),

	/** A pool lists an affix whose own AllowedSubTypes exclude that sub-type. */
	API_RestrictionConflict     UMETA(DisplayName = "Restriction Conflict"),

	/** A pool entry resolves to weight 0, so it is listed but can never roll. */
	API_ZeroWeightEntry         UMETA(DisplayName = "Zero Weight Entry"),
};

USTRUCT(BlueprintType)
struct FAffixPoolIssue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Issue")
	EAffixPoolIssueType Type = EAffixPoolIssueType::API_MissingAffixDefinition;

	UPROPERTY(BlueprintReadOnly, Category = "Issue")
	EAffixPoolIssueSeverity Severity = EAffixPoolIssueSeverity::APS_Warning;

	/** Set the issue was found in, when it belongs to one. */
	UPROPERTY(BlueprintReadOnly, Category = "Issue")
	FName PoolRow;

	UPROPERTY(BlueprintReadOnly, Category = "Issue")
	FName AffixID;

	UPROPERTY(BlueprintReadOnly, Category = "Issue")
	EItemSubType SubType = EItemSubType::IST_None;

	UPROPERTY(BlueprintReadOnly, Category = "Issue")
	FString Detail;

	FString ToString() const;
};

USTRUCT(BlueprintType)
struct FAffixPoolValidationReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Report")
	TArray<FAffixPoolIssue> Issues;

	UPROPERTY(BlueprintReadOnly, Category = "Report")
	int32 SetsChecked = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Report")
	int32 SubTypesWithPools = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Report")
	int32 AffixesDefined = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Report")
	int32 AffixesReachable = 0;

	int32 CountBySeverity(EAffixPoolIssueSeverity Severity) const;
	bool HasErrors() const { return CountBySeverity(EAffixPoolIssueSeverity::APS_Error) > 0; }

	/** Multi-line summary suitable for the output log. */
	FString ToString() const;
};

/**
 * Authoring-time checks for the sub-type affix pools.
 *
 * Per-sub-type pools fail silently by design: forget to list an affix and it
 * simply never rolls, with nothing logged. These checks turn that back into
 * something visible, which is what makes the pools safe to bulk-author.
 *
 * Run from the editor console with `ph.AffixPools.Validate`.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UAffixPoolValidationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Cross-check every pool against the affix definitions.
	 * Any table may be null; checks needing it are skipped.
	 */
	static FAffixPoolValidationReport ValidateAffixPools(
		const UDataTable* PoolTable,
		const UDataTable* PrefixTable,
		const UDataTable* SuffixTable);

	/** Validate the tables a default FAffixGenerator would load. */
	UFUNCTION(BlueprintCallable, Category = "Item|Affix|Validation")
	static FAffixPoolValidationReport ValidateConfiguredAffixPools();

	/**
	 * The complete resolved affix list for one sub-type, includes expanded and
	 * overrides applied - the answer to "what can actually roll on a bow?".
	 */
	UFUNCTION(BlueprintCallable, Category = "Item|Affix|Validation")
	static FString DescribeSubTypePool(EItemSubType SubType);
};
