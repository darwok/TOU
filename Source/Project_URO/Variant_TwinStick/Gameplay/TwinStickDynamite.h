// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TwinStickDynamite.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

/**
 * A dynamite actor thrown by the Desperado enemy.
 * Explodes after a fuse time, or immediately if shot by the player's projectile.
 */
UCLASS()
class PROJECT_URO_API ATwinStickDynamite : public AActor
{
	GENERATED_BODY()
	
public:
	/** Constructor */
	ATwinStickDynamite();

protected:
	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Collision sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionSphere;

	/** Static mesh for visual representation */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

	/** Projectile movement component for parabolic arc throwing */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;

	/** Fuse time in seconds before detonation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dynamite", meta = (ClampMin = 0.1, ClampMax = 10))
	float FuseTime = 2.0f;

	/** Explosion damage radius */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dynamite", meta = (ClampMin = 50, ClampMax = 1500))
	float ExplosionRadius = 250.0f;

	/** Base damage dealt by the explosion */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dynamite", meta = (ClampMin = 0))
	float DamageAmount = 1.0f;

	/** Handle collision hits */
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	/** Timer handle for managing the fuse */
	FTimerHandle FuseTimerHandle;

	/** Detonates the dynamite, dealing radial damage and spawning visuals */
	void Detonate();

	/** Allows Blueprint to handle explosion visual effects and sounds */
	UFUNCTION(BlueprintImplementableEvent, Category = "Dynamite")
	void BP_OnExplosion();
};
