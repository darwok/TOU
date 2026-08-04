// Copyright Epic Games, Inc. All Rights Reserved.

#include "TwinStickDesperadoNPC.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Variant_TwinStick/Gameplay/TwinStickProjectile.h"
#include "Variant_TwinStick/Gameplay/TwinStickDynamite.h"

ATwinStickDesperadoNPC::ATwinStickDesperadoNPC()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // Optimize: Tick is only active when aiming

	// Set default stats
	Score = 15; // Higher score for a mini-boss/advanced enemy
	PickupSpawnChance = 45; // Higher chance to drop power-ups
}

void ATwinStickDesperadoNPC::BeginPlay()
{
	Super::BeginPlay();

	// Cache movement properties
	if (GetCharacterMovement())
	{
		OriginalMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	}

	// Periodically run AI logic checks (tossing dynamite, beginning aim)
	GetWorld()->GetTimerManager().SetTimer(AICheckTimerHandle, this, &ATwinStickDesperadoNPC::ProcessAIStateCheck, 0.2f, true);

	// Cache target player
	TargetPlayer = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
}

void ATwinStickDesperadoNPC::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState == EDesperadoState::Aiming && TargetPlayer)
	{
		CurrentAimTime += DeltaTime;

		// VInterpTo target: adds aiming lag so player can dodge-roll/dash out of lock-on path
		AimTargetLocation = FMath::VInterpTo(AimTargetLocation, TargetPlayer->GetActorLocation(), DeltaTime, 4.0f);

		// Force character to orient toward target while aiming
		FVector DirectionToAim = AimTargetLocation - GetActorLocation();
		DirectionToAim.Z = 0.0f;
		if (!DirectionToAim.IsNearlyZero())
		{
			SetActorRotation(DirectionToAim.Rotation());
		}

		// Laser origin at chest height
		FVector StartLocation = GetActorLocation() + GetActorForwardVector() * 60.0f + FVector(0.0f, 0.0f, 40.0f);

		// Trigger visual updates
		float AimPercent = FMath::Clamp(CurrentAimTime / AimDuration, 0.0f, 1.0f);
		BP_OnLaserUpdate(StartLocation, AimTargetLocation, AimPercent);

		// Render the aiming laser in C++ (Yellow -> Red color shift based on progress)
		FColor LaserColor = FLinearColor::LerpUsingHSV(FLinearColor::Yellow, FLinearColor::Red, AimPercent).ToFColor(true);
		float LaserThickness = 1.0f + (AimPercent * 3.0f); // laser thickens as lock-on completes
		
		DrawDebugLine(GetWorld(), StartLocation, AimTargetLocation, LaserColor, false, -1.0f, 0, LaserThickness);
	}
}

void ATwinStickDesperadoNPC::NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
}

void ATwinStickDesperadoNPC::ProjectileImpact(const FVector& ForwardVector)
{
	// 1. If currently dodge rolling, we are completely invulnerable!
	if (CurrentState == EDesperadoState::DodgeRolling)
	{
		return;
	}

	// 2. If we get hit and dodge is off cooldown, trigger a Dodge Roll to avoid subsequent damage
	if (!bDodgeOnCooldown && !bHit)
	{
		FVector DodgeDir = FVector::CrossProduct(GetActorForwardVector(), FVector::UpVector);
		if (FMath::RandBool())
		{
			DodgeDir = -DodgeDir;
		}
		DodgeDir.Normalize();

		StartDodgeRoll(DodgeDir);
	}

	// 3. Process base damage/hit logic
	Super::ProjectileImpact(ForwardVector);
}

void ATwinStickDesperadoNPC::ProcessAIStateCheck()
{
	// Do not run decisions if dead, inactive, stunned, or busy dodge-rolling
	if (bHit || bStunned || !bActiveInWorld || CurrentState == EDesperadoState::DodgeRolling || !TargetPlayer)
	{
		return;
	}

	float DistanceToPlayer = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());

	// If player is out of reach, stay idle/normal
	if (DistanceToPlayer > SniperMaxRange)
	{
		return;
	}

	// Decision 1: Try to throw dynamite (if player is not too close and dynamite is off cooldown)
	if (DistanceToPlayer >= MinDynamiteTossDistance && !bDynamiteOnCooldown && CurrentState == EDesperadoState::Idle)
	{
		// 35% chance to toss dynamite when checking, to vary AI behaviors
		if (FMath::RandRange(0, 100) < 35)
		{
			TossDynamite();
			return;
		}
	}

	// Decision 2: Start sniper aiming
	if (!bSniperOnCooldown && CurrentState == EDesperadoState::Idle)
	{
		EnterAimingState();
	}
}

void ATwinStickDesperadoNPC::EnterAimingState()
{
	CurrentState = EDesperadoState::Aiming;
	CurrentAimTime = 0.0f;
	AimTargetLocation = TargetPlayer->GetActorLocation();

	// Temporarily halt movement while aiming
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = 0.0f;
		GetCharacterMovement()->Velocity = FVector::ZeroVector;
	}

	// Activate laser visuals and tick
	BP_SetLaserActive(true);
	SetActorTickEnabled(true);

	// Set timer to fire sniper shot when lock-on ends
	GetWorld()->GetTimerManager().SetTimer(StateTimerHandle, this, &ATwinStickDesperadoNPC::FireSniperShot, AimDuration, false);
}

void ATwinStickDesperadoNPC::FireSniperShot()
{
	// Clear aim state
	SetActorTickEnabled(false);
	BP_SetLaserActive(false);

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed;
	}

	// Spawn Sniper bullet pointing at locked-on target
	if (SniperProjectileClass && TargetPlayer && !bHit)
	{
		FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * 85.0f + FVector(0.0f, 0.0f, 40.0f);
		FVector FireDir = AimTargetLocation - SpawnLocation;
		FireDir.Z = 0.0f;
		FireDir.Normalize();

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ATwinStickProjectile* Projectile = GetWorld()->SpawnActor<ATwinStickProjectile>(
			SniperProjectileClass, 
			SpawnLocation, 
			FireDir.Rotation(), 
			SpawnParams
		);

		if (Projectile)
		{
			// Give the projectile a high-velocity boost
			UProjectileMovementComponent* ProjMov = Projectile->FindComponentByClass<UProjectileMovementComponent>();
			if (ProjMov)
			{
				ProjMov->InitialSpeed = SniperShotSpeed;
				ProjMov->MaxSpeed = SniperShotSpeed * 1.5f;
				ProjMov->Velocity = FireDir * SniperShotSpeed;
				ProjMov->UpdateComponentVelocity();
			}
		}
	}

	// Put sniper on cooldown
	bSniperOnCooldown = true;
	GetWorld()->GetTimerManager().SetTimer(SniperCooldownTimerHandle, this, &ATwinStickDesperadoNPC::ResetSniperCooldown, SniperCooldown, false);

	CurrentState = EDesperadoState::Idle;
}

void ATwinStickDesperadoNPC::TossDynamite()
{
	if (!DynamiteClass || !TargetPlayer || bHit)
	{
		return;
	}

	FVector StartLoc = GetActorLocation() + GetActorForwardVector() * 60.0f + FVector(0.0f, 0.0f, 80.0f); // Toss from above shoulder
	FVector TargetLoc = TargetPlayer->GetActorLocation();

	// Calculate parabolic trajectory velocity
	FVector LaunchVelocity;
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	bool bFoundVelocity = UGameplayStatics::SuggestProjectileVelocity(
		this,
		LaunchVelocity,
		StartLoc,
		TargetLoc,
		800.0f,
		false,
		0.0f,
		0.0f,
		ESuggestProjVelocityTraceOption::DoNotTrace
	);
PRAGMA_ENABLE_DEPRECATION_WARNINGS

	// Fallback to simple direct/angled throw if calculation fails
	if (!bFoundVelocity)
	{
		FVector Dir = TargetLoc - StartLoc;
		Dir.Z = 0.0f;
		Dir.Normalize();
		LaunchVelocity = (Dir * 700.0f) + FVector(0.0f, 0.0f, 450.0f);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATwinStickDynamite* Dynamite = GetWorld()->SpawnActor<ATwinStickDynamite>(
		DynamiteClass,
		StartLoc,
		LaunchVelocity.Rotation(),
		SpawnParams
	);

	if (Dynamite)
	{
		UProjectileMovementComponent* ProjMov = Dynamite->FindComponentByClass<UProjectileMovementComponent>();
		if (ProjMov)
		{
			ProjMov->Velocity = LaunchVelocity;
			ProjMov->UpdateComponentVelocity();
		}
	}

	// Trigger audio-visual events
	BP_OnDynamiteThrown();

	// Put dynamite on cooldown
	bDynamiteOnCooldown = true;
	GetWorld()->GetTimerManager().SetTimer(DynamiteCooldownTimerHandle, this, &ATwinStickDesperadoNPC::ResetDynamiteCooldown, DynamiteCooldown, false);
}

void ATwinStickDesperadoNPC::StartDodgeRoll(const FVector& DodgeDirection)
{
	CurrentState = EDesperadoState::DodgeRolling;

	// Interrupt aiming if we were locked on
	GetWorld()->GetTimerManager().ClearTimer(StateTimerHandle);
	SetActorTickEnabled(false);
	BP_SetLaserActive(false);

	// Temporarily boost max walk speed and force movement velocity
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed * DodgeSpeedMultiplier;
		GetCharacterMovement()->Velocity = DodgeDirection * (OriginalMaxWalkSpeed * DodgeSpeedMultiplier);
	}

	// Rotate character to face the roll direction
	if (!DodgeDirection.IsNearlyZero())
	{
		SetActorRotation(DodgeDirection.Rotation());
	}

	// Trigger visual effects/animations
	BP_OnDodgeRollStart(DodgeDirection);

	// Timer to end the dodge roll
	GetWorld()->GetTimerManager().SetTimer(StateTimerHandle, this, &ATwinStickDesperadoNPC::EndDodgeRoll, DodgeDuration, false);

	// Set dodge roll cooldown
	bDodgeOnCooldown = true;
	GetWorld()->GetTimerManager().SetTimer(DodgeCooldownTimerHandle, this, &ATwinStickDesperadoNPC::ResetDodgeCooldown, DodgeCooldown, false);
}

void ATwinStickDesperadoNPC::EndDodgeRoll()
{
	// Restore standard speed
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed;
	}

	CurrentState = EDesperadoState::Idle;

	BP_OnDodgeRollEnd();
}

void ATwinStickDesperadoNPC::ApplyLassoStun(float Duration)
{
	// Interrupt sniper aiming if we are currently aiming
	if (CurrentState == EDesperadoState::Aiming)
	{
		GetWorld()->GetTimerManager().ClearTimer(StateTimerHandle);
		SetActorTickEnabled(false);
		BP_SetLaserActive(false);
		CurrentState = EDesperadoState::Idle;
	}

	// Call parent class to halt movement and pause AI brain
	Super::ApplyLassoStun(Duration);
}
