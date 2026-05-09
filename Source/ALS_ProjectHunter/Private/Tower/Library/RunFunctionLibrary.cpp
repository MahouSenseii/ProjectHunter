// Tower/Library/RunFunctionLibrary.cpp

#include "Tower/Library/RunFunctionLibrary.h"
#include "Tower/Subsystem/RunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogRunFunctionLibrary);

// ─────────────────────────────────────────────────────────────────────────────
// SUBSYSTEM ACCESS
// ─────────────────────────────────────────────────────────────────────────────

URunSubsystem* URunFunctionLibrary::GetRunSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GI = World->GetGameInstance();
	return GI ? GI->GetSubsystem<URunSubsystem>() : nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// STATE QUERIES
// ─────────────────────────────────────────────────────────────────────────────

ERunState URunFunctionLibrary::GetRunState(const UObject* WorldContextObject)
{
	const URunSubsystem* RS = GetRunSubsystem(WorldContextObject);
	if (!RS)
	{
		return ERunState::Inactive;
	}

	return RS->IsRunActive() ? ERunState::Active : ERunState::Inactive;
}

bool URunFunctionLibrary::IsRunActive(const UObject* WorldContextObject)
{
	const URunSubsystem* RS = GetRunSubsystem(WorldContextObject);
	return RS ? RS->IsRunActive() : false;
}

int32 URunFunctionLibrary::GetCurrentFloor(const UObject* WorldContextObject)
{
	const URunSubsystem* RS = GetRunSubsystem(WorldContextObject);
	return RS ? RS->GetCurrentFloor() : 0;
}

int32 URunFunctionLibrary::GetTotalKills(const UObject* WorldContextObject)
{
	const URunSubsystem* RS = GetRunSubsystem(WorldContextObject);
	return RS ? RS->GetTotalKills() : 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// FORMATTING
// ─────────────────────────────────────────────────────────────────────────────

FString URunFunctionLibrary::FormatRunTime(float Seconds)
{
	const int32 TotalSeconds = FMath::FloorToInt(FMath::Max(Seconds, 0.f));
	const int32 Minutes      = TotalSeconds / 60;
	const int32 Secs         = TotalSeconds % 60;

	return FString::Printf(TEXT("%02d:%02d"), Minutes, Secs);
}
