#include "Tower/Subsystems/PortalTravelSubsystem.h"

void UPortalTravelSubsystem::SetPendingArrival(
	const FName DestinationLevel,
	const FName ArrivalPortalID,
	const int32 ExpectedTravellers)
{
	PendingDestinationLevel = DestinationLevel;
	PendingArrivalPortalID = ArrivalPortalID;
	ExpectedTravellerCount = FMath::Max(1, ExpectedTravellers);
	bHasPendingArrival = ArrivalPortalID != NAME_None;
}

bool UPortalTravelSubsystem::MatchesArrivalPortal(const FName PortalID) const
{
	return bHasPendingArrival && PortalID != NAME_None && PortalID == PendingArrivalPortalID;
}

void UPortalTravelSubsystem::ClearPendingArrival()
{
	PendingDestinationLevel = NAME_None;
	PendingArrivalPortalID = NAME_None;
	ExpectedTravellerCount = 0;
	bHasPendingArrival = false;
}
