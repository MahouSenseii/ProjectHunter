// Tower/Subsystem/PortalSubsystem.cpp
#include "Tower/Subsystem/PortalSubsystem.h"
#include "Interactable/Actors/Portal/PortalActor.h"

DEFINE_LOG_CATEGORY(LogPortalSubsystem);

void UPortalSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogPortalSubsystem, Log, TEXT("PortalSubsystem initialised"));
}

void UPortalSubsystem::Deinitialize()
{
	PortalRegistry.Empty();
	Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────

void UPortalSubsystem::RegisterPortal(FName PortalID, APortalActor* Portal)
{
	if (PortalID == NAME_None || !Portal)
	{
		UE_LOG(LogPortalSubsystem, Warning,
			TEXT("RegisterPortal: Invalid PortalID or null portal pointer"));
		return;
	}

	if (PortalRegistry.Contains(PortalID))
	{
		UE_LOG(LogPortalSubsystem, Warning,
			TEXT("RegisterPortal: Portal '%s' is already registered — overwriting. "
				 "Check for duplicate PortalIDs in the level."),
			*PortalID.ToString());
	}

	PortalRegistry.Add(PortalID, Portal);

	UE_LOG(LogPortalSubsystem, Log,
		TEXT("RegisterPortal: Registered '%s' (%d total)"),
		*PortalID.ToString(), PortalRegistry.Num());
}

void UPortalSubsystem::UnregisterPortal(FName PortalID)
{
	if (PortalRegistry.Remove(PortalID) > 0)
	{
		UE_LOG(LogPortalSubsystem, Log,
			TEXT("UnregisterPortal: Removed '%s' (%d remaining)"),
			*PortalID.ToString(), PortalRegistry.Num());
	}
}

APortalActor* UPortalSubsystem::FindPortal(FName PortalID) const
{
	const TWeakObjectPtr<APortalActor>* Found = PortalRegistry.Find(PortalID);
	if (!Found)
	{
		return nullptr;
	}
	return Found->IsValid() ? Found->Get() : nullptr;
}

TArray<APortalActor*> UPortalSubsystem::GetAllPortals() const
{
	TArray<APortalActor*> Result;
	Result.Reserve(PortalRegistry.Num());

	for (const auto& Pair : PortalRegistry)
	{
		if (Pair.Value.IsValid())
		{
			Result.Add(Pair.Value.Get());
		}
	}
	return Result;
}
