// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"

#include "TwinStickStateTreeUtility.generated.h"

class ACharacter;

/**
 *  Instance data struct for the Get Player task
 */
USTRUCT()
struct FStateTreeGetPlayerInstanceData
{
	GENERATED_BODY()

	/** Character that owns this task */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ACharacter> Character;

	/** Character that owns this task */
	UPROPERTY(VisibleAnywhere, Category = "Output")
	TObjectPtr<ACharacter> TargetPlayerCharacter;
};

/**
 *  StateTree task to get the player character
 */
USTRUCT(meta = (DisplayName = "GetPlayer", Category = "TwinStick"))
struct FStateTreeGetPlayerTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* Ensure we're using the correct instance data struct */
	using FInstanceDataType = FStateTreeGetPlayerInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Runs while the owning state is active */
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};

/**
 *  Instance data struct for the Get Random Location task
 */
USTRUCT()
struct FStateTreeGetRandomLocationInstanceData
{
	GENERATED_BODY()

	/** Actor that is moving */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor;

	/** Search radius for patrol */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float Radius = 800.0f;

	/** Output destination */
	UPROPERTY(VisibleAnywhere, Category = "Output")
	FVector TargetLocation = FVector::ZeroVector;
};

/**
 *  StateTree task to find a random location on the NavMesh
 */
USTRUCT(meta = (DisplayName = "Get Random Location", Category = "TwinStick"))
struct FStateTreeGetRandomLocationTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeGetRandomLocationInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};