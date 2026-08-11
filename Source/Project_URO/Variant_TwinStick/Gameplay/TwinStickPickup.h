// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TwinStickPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EPickupType : uint8
{
	BombItem,
	SGun,
	MGun
};

/**
 *  A simple pickup for a Twin Stick Shooter game
 */
UCLASS(abstract)
class ATwinStickPickup : public AActor
{
	GENERATED_BODY()
	
	/** Pickup collision sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionSphere;

	/** Provides visual representation for the pickup */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

protected:
	/** Type of pickup */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	EPickupType PickupType = EPickupType::BombItem;

	/** Ammo amount granted by this pickup if it is a weapon */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = 1))
	int32 AmmoAmount = 30;

public:	

	/** Constructor */
	ATwinStickPickup();

	/** Collision handling */
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

};
