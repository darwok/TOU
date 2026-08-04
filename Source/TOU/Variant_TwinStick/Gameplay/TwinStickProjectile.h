// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pooling/PoolableActor.h"
#include "TwinStickProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UActorPool;

/**
 *  A simple bouncing projectile for a Twin Stick shooter game
 */
UCLASS(abstract)
class ATwinStickProjectile : public AActor, public IPoolableActor
{
	GENERATED_BODY()
	
	/** Projectile collision sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionSphere;

	/** Mesh that provides the visual representation for this projectile */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

	/** Handles movement behaviors for this projectile */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;

public:	

	/** Constructor */
	ATwinStickProjectile();

	/** Handles collisions */
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	/** The pool that owns this projectile */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Pooling")
	UActorPool* OwningPool;

	// IPoolableActor interface implementation
	virtual void OnActivatedFromPool_Implementation() override;
	virtual void OnReturnedToPool_Implementation() override;

	/** Devolves the actor to the pool or destroys it */
	void ReturnToPool();

protected:
	
	/** Handles collisions that stop this projectile from moving */
	UFUNCTION()
	void OnProjectileStop(const FHitResult& ImpactResult);

	/** Timer handle for managing lifespan without using InitialLifeSpan */
	FTimerHandle LifeSpanTimerHandle;

};
