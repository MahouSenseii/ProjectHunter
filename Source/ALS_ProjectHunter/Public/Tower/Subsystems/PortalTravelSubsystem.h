#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PortalTravelSubsystem.generated.h"

/**
 * Persists a portal arrival request across OpenLevel/ServerTravel. The destination
 * portal resolves the request after the new world's player pawns are available.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPortalTravelSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void SetPendingArrival(FName DestinationLevel, FName ArrivalPortalID, int32 ExpectedTravellers);
	bool MatchesArrivalPortal(FName PortalID) const;
	void ClearPendingArrival();

	FName GetPendingArrivalPortalID() const { return PendingArrivalPortalID; }
	FName GetPendingDestinationLevel() const { return PendingDestinationLevel; }
	int32 GetExpectedTravellerCount() const { return ExpectedTravellerCount; }
	bool HasPendingArrival() const { return bHasPendingArrival; }

private:
	UPROPERTY(Transient)
	FName PendingDestinationLevel = NAME_None;

	UPROPERTY(Transient)
	FName PendingArrivalPortalID = NAME_None;

	UPROPERTY(Transient)
	int32 ExpectedTravellerCount = 0;

	UPROPERTY(Transient)
	bool bHasPendingArrival = false;
};
