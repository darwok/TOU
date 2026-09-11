// Copyright Epic Games, Inc. All Rights Reserved.

#include "TwinStickStateTreeUtility.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeExecutionTypes.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

#define LOCTEXT_NAMESPACE "TopDownTemplate"

EStateTreeRunStatus FStateTreeGetPlayerTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// get the instance data
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// get the pawn possessed by the first local player
	InstanceData.TargetPlayerCharacter = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(InstanceData.Character, 0));

	// keep the task running
	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FStateTreeGetPlayerTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return LOCTEXT("StateTreeTaskGetPlayerDescription", "<b>Get Player</b>");
}
#endif // WITH_EDITOR


EStateTreeRunStatus FStateTreeGetRandomLocationTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.Actor)
	{
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(InstanceData.Actor->GetWorld());
		if (NavSys)
		{
			FVector ResultLoc;
			// Busca un punto aleatorio navegable alrededor del enemigo
			if (NavSys->K2_GetRandomReachablePointInRadius(InstanceData.Actor->GetWorld(), InstanceData.Actor->GetActorLocation(), ResultLoc, InstanceData.Radius))
			{
				InstanceData.TargetLocation = ResultLoc;
				return EStateTreeRunStatus::Running;
			}
		}
	}
	return EStateTreeRunStatus::Failed;
}

#if WITH_EDITOR
FText FStateTreeGetRandomLocationTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return LOCTEXT("StateTreeTaskGetRandomLocationDescription", "<b>Get Random Location</b>");
}
#endif // WITH_EDITOR