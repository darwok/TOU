// Copyright Epic Games, Inc. All Rights Reserved.


#include "TwinStickNPC.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TwinStickCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TwinStickGameMode.h"
#include "TwinStickPickup.h"
#include "Engine/World.h"
#include "TwinStickNPCDestruction.h"
#include "TimerManager.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Pooling/ActorPool.h"

ATwinStickNPC::ATwinStickNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	// ensure we spawn an AI controller when we're spawned
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// configure the inherited components
	GetCapsuleComponent()->SetCapsuleRadius(45.0f);
	GetCapsuleComponent()->SetNotifyRigidBodyCollision(true);

	GetMesh()->SetCollisionProfileName(FName("NoCollision"));

	GetCharacterMovement()->GravityScale = 1.5f;
	GetCharacterMovement()->MaxAcceleration = 1000.0f;
	GetCharacterMovement()->BrakingFriction = 1.0f;
	GetCharacterMovement()->MaxWalkSpeed = 200.0f;
	GetCharacterMovement()->MaxWalkSpeedCrouched = 100.0f;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 250.0f;
	GetCharacterMovement()->AvoidanceWeight = 1.0f;
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
}

void ATwinStickNPC::BeginPlay()
{
	Super::BeginPlay();

	// Mark active and increment counter
	bActiveInWorld = true;
	if (ATwinStickGameMode* GM = Cast<ATwinStickGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->IncreaseNPCs();
	}

}

void ATwinStickNPC::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the destruction timer
	GetWorld()->GetTimerManager().ClearTimer(DestructionTimer);
}

void ATwinStickNPC::Destroyed()
{
	// decrease the NPC counter so we can cap spawning if necessary if we were active
	if (bActiveInWorld)
	{
		bActiveInWorld = false;
		if (ATwinStickGameMode* GM = Cast<ATwinStickGameMode>(GetWorld()->GetAuthGameMode()))
		{
			GM->DecreaseNPCs();
		}
	}

	Super::Destroyed();
}

void ATwinStickNPC::NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	// have we collided against the player?
	if (ATwinStickCharacter* PlayerCharacter = Cast<ATwinStickCharacter>(Other))
	{
		// apply damage to the character
		PlayerCharacter->HandleDamage(1.0f, GetActorForwardVector());
	}
}

void ATwinStickNPC::ProjectileImpact(const FVector& ForwardVector)
{
	// only handle damage if we haven't been hit yet
	if (bHit)
	{
		return;
	}

	// raise the hit flag
	bHit = true;

	// deactivate character movement
	GetCharacterMovement()->Deactivate();

	// award points
	if (ATwinStickGameMode* GM = Cast<ATwinStickGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->ScoreUpdate(Score);
	}

	// randomly spawn a pickup
	if (FMath::RandRange(0, 100) < PickupSpawnChance)
	{
		ATwinStickPickup* Pickup = GetWorld()->SpawnActor<ATwinStickPickup>(PickupClass, GetActorTransform());
	}
	
	// spawn the NPC destruction proxy
	ATwinStickNPCDestruction* DestructionProxy = GetWorld()->SpawnActor<ATwinStickNPCDestruction>(DestructionProxyClass, GetActorTransform());

	// hide this actor
	SetActorHiddenInGame(true);

	// disable collision
	SetActorEnableCollision(false);

	// defer destruction
	GetWorld()->GetTimerManager().SetTimer(DestructionTimer, this, &ATwinStickNPC::DeferredDestroy, DeferredDestructionTime, false);
}

void ATwinStickNPC::DeferredDestroy()
{
	if (OwningPool)
	{
		OwningPool->ReturnActorToPool(this);
	}
	else
	{
		Destroy();
	}
}

void ATwinStickNPC::OnActivatedFromPool_Implementation()
{
	// 1. Reset state
	bHit = false;

	// 2. Reactivate movement component
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->Activate(true);
		GetCharacterMovement()->Velocity = FVector::ZeroVector;
	}

	// 3. Restart AI logic
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* BrainComp = AIC->GetBrainComponent())
		{
			BrainComp->RestartLogic();
		}
	}

	// 4. Update active state and increment active count
	if (!bActiveInWorld)
	{
		bActiveInWorld = true;
		if (ATwinStickGameMode* GM = Cast<ATwinStickGameMode>(GetWorld()->GetAuthGameMode()))
		{
			GM->IncreaseNPCs();
		}
	}
}

void ATwinStickNPC::OnReturnedToPool_Implementation()
{
	// 1. Clear destruction timer
	GetWorld()->GetTimerManager().ClearTimer(DestructionTimer);

	// Clear stun state and timer
	if (bStunned)
	{
		GetWorld()->GetTimerManager().ClearTimer(StunTimerHandle);
		bStunned = false;
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->MaxWalkSpeed = OriginalWalkSpeed;
		}
	}

	// 2. Stop AI Controller logic
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* BrainComp = AIC->GetBrainComponent())
		{
			BrainComp->StopLogic(TEXT("Returned to pool"));
		}
	}

	// 3. Update active state and decrement active count
	if (bActiveInWorld)
	{
		bActiveInWorld = false;
		if (ATwinStickGameMode* GM = Cast<ATwinStickGameMode>(GetWorld()->GetAuthGameMode()))
		{
			GM->DecreaseNPCs();
		}
	}
}

void ATwinStickNPC::ApplyLassoStun(float Duration)
{
	if (bHit)
	{
		return;
	}

	bStunned = true;

	// Pause AI Logic
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* BrainComp = AIC->GetBrainComponent())
		{
			BrainComp->PauseLogic(TEXT("Stunned by Lasso"));
		}
	}

	// Halt movement
	if (GetCharacterMovement())
	{
		OriginalWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
		GetCharacterMovement()->MaxWalkSpeed = 0.0f;
		GetCharacterMovement()->Velocity = FVector::ZeroVector;
	}

	// Start stun timer
	GetWorld()->GetTimerManager().SetTimer(StunTimerHandle, this, &ATwinStickNPC::EndLassoStun, Duration, false);
}

void ATwinStickNPC::EndLassoStun()
{
	if (!bStunned)
	{
		return;
	}

	bStunned = false;

	// Restore movement
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = OriginalWalkSpeed;
	}

	// Resume AI Logic
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* BrainComp = AIC->GetBrainComponent())
		{
			BrainComp->ResumeLogic(TEXT("Stun recovered"));
		}
	}
}
