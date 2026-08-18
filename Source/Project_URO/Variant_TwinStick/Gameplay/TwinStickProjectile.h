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

	/** Material de decal a utilizar en el impacto */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decals")
	TObjectPtr<UMaterialInterface> DecalMaterial;

	/** Tamaño del decal de impacto */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decals")
	FVector DecalSize = FVector(128.0f, 32.0f, 32.0f);

	/** Duración (tiempo de vida) del decal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decals")
	float DecalLifeSpan = 5.0f;

	/** Bandera interna para evitar duplicación de decals en una misma activación */
	bool bDecalSpawned = false;

};
