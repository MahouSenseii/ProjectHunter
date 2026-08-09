#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PHPlayerState.generated.h"

class URunSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogPHPlayerState, Log, All);

UCLASS()
class ALS_PROJECTHUNTER_API APHPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	APHPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Team")
	uint8 TeamID = 1;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Team")
	void SetTeamID(uint8 NewTeamID);

	UFUNCTION(BlueprintPure, Category = "Team")
	uint8 GetTeamID() const { return TeamID; }

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Run")
	int32 CurrentFloor = 1;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Run")
	int32 RunKillCount = 0;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Run")
	void AdvanceFloor();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Run")
	void RecordKill();

	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Save")
	FString CharacterSlotName;

protected:
	URunSubsystem* GetRunSubsystem() const;
};
