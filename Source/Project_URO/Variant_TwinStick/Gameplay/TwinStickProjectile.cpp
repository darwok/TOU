// Copyright Epic Games, Inc. All Rights Reserved.


#include "TwinStickProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TwinStickNPC.h"
#include "Pooling/ActorPool.h"
#include "TimerManager.h"
#include "Engine/World.h"

ATwinStickProjectile::ATwinStickProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// set InitialLifeSpan to 0 so the actor is not destroyed on spawn when pooled
	InitialLifeSpan = 0.0f;

	// create the collision sphere and set it as the root component
	RootComponent = CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Sphere"));

	CollisionSphere->SetSphereRadius(35.0f);
	CollisionSphere->SetNotifyRigidBodyCollision(true);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);

	// create the mesh
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

	Mesh->SetCollisionProfileName(FName("NoCollision"));

	// create the projectile movement comp. No need to attach it because it's not a scene component
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));

	ProjectileMovement->InitialSpeed = 2000.0f;
	ProjectileMovement->MaxSpeed = 15000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bRotationRemainsVertical = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bForceSubStepping = true;

	ProjectileMovement->OnProjectileStop.AddDynamic(this, &ATwinStickProjectile::OnProjectileStop);
}

void ATwinStickProjectile::NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	// have we hit a NPC?
	if (ATwinStickNPC* NPC = Cast<ATwinStickNPC>(Other))
	{
		// tell the NPC it's been hit
		NPC->ProjectileImpact(FVector::ZeroVector);

		// return to pool instead of destroying
		ReturnToPool();
	}
}

void ATwinStickProjectile::OnProjectileStop(const FHitResult& ImpactResult)
{
	// return to pool instead of destroying
	ReturnToPool();
}

void ATwinStickProjectile::OnActivatedFromPool_Implementation()
{
	if (GetOwner())
	{
		CollisionSphere->IgnoreActorWhenMoving(GetOwner(), true);
	}

	// Reset/activate projectile movement
	if (ProjectileMovement)
	{
		ProjectileMovement->Activate(true);
		ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileMovement->InitialSpeed;
		ProjectileMovement->UpdateComponentVelocity();
	}

	// Recreate lifespan timer (2.0s duration)
	GetWorld()->GetTimerManager().SetTimer(LifeSpanTimerHandle, this, &ATwinStickProjectile::ReturnToPool, 2.0f, false);
}

void ATwinStickProjectile::OnReturnedToPool_Implementation()
{
	// Clear the lifespan timer when returned to the pool
	GetWorld()->GetTimerManager().ClearTimer(LifeSpanTimerHandle);
}

void ATwinStickProjectile::ReturnToPool()
{
	// Stop lifespan timer
	GetWorld()->GetTimerManager().ClearTimer(LifeSpanTimerHandle);

	if (OwningPool)
	{
		OwningPool->ReturnActorToPool(this);
	}
	else
	{
		Destroy();
	}
}
