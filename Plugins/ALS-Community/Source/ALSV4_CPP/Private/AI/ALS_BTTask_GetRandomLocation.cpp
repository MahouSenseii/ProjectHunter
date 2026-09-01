// Copyright:       Copyright (C) 2022 Doğa Can Yanıkoğlu
// Source Code:     https://github.com/dyanikoglu/ALS-Community

#include "AI/ALS_BTTask_GetRandomLocation.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "NavFilters/NavigationQueryFilter.h"

UALS_BTTask_GetRandomLocation::UALS_BTTask_GetRandomLocation()
{
	NodeName = "Get Random Location";

	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UALS_BTTask_GetRandomLocation, BlackboardKey));
}

EBTNodeResult::Type UALS_BTTask_GetRandomLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!IsValid(AIController) || !IsValid(Blackboard))
	{
		return EBTNodeResult::Failed;
	}

	APawn* Pawn = AIController->GetPawn();
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(OwnerComp.GetWorld());
	if (!IsValid(Pawn) || !IsValid(NavSys))
	{
		return EBTNodeResult::Failed;
	}

	ANavigationData* NavData = NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
	if (!IsValid(NavData))
	{
		return EBTNodeResult::Failed;
	}

	FSharedConstNavQueryFilter SharedFilter = nullptr;
	if (Filter)
	{
		// Controller-dependent meta filters require the querying agent, not the world.
		SharedFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, AIController, Filter);
	}

	FNavLocation Destination;
	if (!NavSys->GetRandomReachablePointInRadius(Pawn->GetActorLocation(), MaxDistance, Destination, NavData, SharedFilter))
	{
		return EBTNodeResult::Failed;
	}

	return Blackboard->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.SelectedKeyName, Destination.Location)
		? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

FString UALS_BTTask_GetRandomLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("Get Random Location\nMax Distance: %d\nFilter:%s"), FMath::RoundToInt(MaxDistance),
	                       Filter ? *GetNameSafe(Filter.Get()) : TEXT("None"));
}
