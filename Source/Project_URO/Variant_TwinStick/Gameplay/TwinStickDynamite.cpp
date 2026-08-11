// Copyright Epic Games, Inc. All Rights Reserved.

#include "TwinStickDynamite.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"
#include "Variant_TwinStick/TwinStickCharacter.h"
#include "Variant_TwinStick/AI/TwinStickNPC.h"
#include "Variant_TwinStick/Gameplay/TwinStickProjectile.h"

ATwinStickDynamite::ATwinStickDynamite()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create and configure collision sphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->SetSphereRadius(30.0f);
	CollisionSphere->SetNotifyRigidBodyCollision(true);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);

	// Create and attach mesh
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionProfileName(FName("NoCollision"));

	// Create projectile movement component for parabolas
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 800.0f;
	ProjectileMovement->MaxSpeed = 2000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 1.8f; // High gravity for quick toss arc
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.25f;
}

void ATwinStickDynamite::BeginPlay()
{
	Super::BeginPlay();

	// Start the fuse timer
	GetWorld()->GetTimerManager().SetTimer(FuseTimerHandle, this, &ATwinStickDynamite::Detonate, FuseTime, false);
}

void ATwinStickDynamite::NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	// Did we get hit by a player's projectile?
	if (ATwinStickProjectile* Projectile = Cast<ATwinStickProjectile>(Other))
	{
		// Detonate immediately
		GetWorld()->GetTimerManager().ClearTimer(FuseTimerHandle);
		Detonate();

		// Return projectile to pool
		Projectile->ReturnToPool();
	}
}

void ATwinStickDynamite::Detonate()
{
	// Ensure the timer is cleared
	GetWorld()->GetTimerManager().ClearTimer(FuseTimerHandle);

	// Trigger Blueprint explosion effects (particles/sounds)
	BP_OnExplosion();

	// Perform radial overlap check to deal damage
	TArray<FOverlapResult> Overlaps;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(ExplosionRadius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bHitAnything = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ECC_Pawn, // Checks against pawns (Player and NPCs)
		SphereShape,
		QueryParams
	);

	if (bHitAnything)
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* OverlapActor = Overlap.GetActor();
			if (!OverlapActor)
			{
				continue;
			}

			// If it's the player, deal damage
			if (ATwinStickCharacter* Player = Cast<ATwinStickCharacter>(OverlapActor))
			{
				FVector DamageDirection = Player->GetActorLocation() - GetActorLocation();
				DamageDirection.Normalize();
				Player->HandleDamage(DamageAmount, DamageDirection);
			}
			// If it's another NPC, deal damage (friendly fire / tactical use)
			else if (ATwinStickNPC* NPC = Cast<ATwinStickNPC>(OverlapActor))
			{
				// ProjectileImpact expects ForwardVector, we can pass direction from explosion
				FVector DamageDirection = NPC->GetActorLocation() - GetActorLocation();
				DamageDirection.Normalize();
				NPC->ProjectileImpact(DamageDirection);
			}
		}
	}

	// Destroy the dynamite actor
	Destroy();
}
