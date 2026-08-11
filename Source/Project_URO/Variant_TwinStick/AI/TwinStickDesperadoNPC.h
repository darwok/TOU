// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Variant_TwinStick/AI/TwinStickNPC.h"
#include "TwinStickDesperadoNPC.generated.h"

class ATwinStickProjectile;
class ATwinStickDynamite;

/**
 * State enum for the Desperado enemy.
 */
UENUM(BlueprintType)
enum class EDesperadoState : uint8
{
	Idle,
	Aiming,
	Firing,
	DodgeRolling
};

/**
 * A highly optimized, Wild Guns-inspired Desperado enemy.
 * Uses a timer-driven state machine to avoid ticking when idle, and implements
 * laser-sighted sniper shots, dynamite tossing, and active dodge-rolling.
 */
UCLASS()
class PROJECT_URO_API ATwinStickDesperadoNPC : public ATwinStickNPC
{
	GENERATED_BODY()

public:
	/** Constructor */
	ATwinStickDesperadoNPC();

	// IPoolableActor interface implementation overrides
	virtual void OnActivatedFromPool_Implementation() override;
	virtual void OnReturnedToPool_Implementation() override;

protected:
	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Frame-by-frame updates (only used when actively aiming) */
	virtual void Tick(float DeltaTime) override;

	/** Overrides NPC hit detection to trigger dodge-rolls */
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	/** Overrides NPC projectile impact to handle invulnerability during dodge rolls */
	virtual void ProjectileImpact(const FVector& ForwardVector) override;

	/** Overrides ApplyLassoStun to interrupt laser aiming */
	virtual void ApplyLassoStun(float Duration) override;

	// --- Custom AI Config ---

	/** How long the Desperado locks onto the player before firing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Desperado|Sniper", meta = (ClampMin = 0.1, ClampMax = 5.0))
	float AimDuration = 1.2f;

	/** Maximum range to start aiming at the player */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Desperado|Sniper", meta = (ClampMin = 100))
	float SniperMaxRange = 1200.0f;

	/** Class of the high-velocity projectile to fire */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Desperado|Sniper")
	TSubclassOf<ATwinStickProjectile> SniperProjectileClass;

	/** Projectile speed for the sniper shot */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Desperado|Sniper", meta = (ClampMin = 500))
	float SniperShotSpeed = 3500.0f;

	/** Delay between sniper shots */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Desperado|Sniper", meta = (ClampMin = 0.5))
	float SniperCooldown = 3.5f;

	/** Class of the dynamite to toss */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Desperado|Dynamite")
	TSubclassOf<ATwinStickDynamite> DynamiteClass;

	/** Cooldown for throwing dynamite */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Desperado|Dynamite", meta = (ClampMin = 1.0))
	float DynamiteCooldown = 6.0f;

	/** Minimum distance to toss dynamite */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Desperado|Dynamite", meta = (ClampMin = 200))
	float MinDynamiteTossDistance = 300.0f;

	/** Cooldown for performing a dodge roll */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Desperado|Dodge", meta = (ClampMin = 0.5))
	float DodgeCooldown = 4.0f;

	/** Speed multiplier during a dodge roll */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Desperado|Dodge", meta = (ClampMin = 1.0, ClampMax = 5.0))
	float DodgeSpeedMultiplier = 2.2f;

	/** Duration of the dodge roll */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Desperado|Dodge", meta = (ClampMin = 0.1, ClampMax = 1.5))
	float DodgeDuration = 0.4f;

	// --- Blueprint Interface Events ---

	/** Called when laser sight updates. Can be used for custom particle or visual lines in BP */
	UFUNCTION(BlueprintImplementableEvent, Category = "Desperado")
	void BP_OnLaserUpdate(FVector Start, FVector End, float AimProgressPercent);

	/** Called when starting/stopping the laser */
	UFUNCTION(BlueprintImplementableEvent, Category = "Desperado")
	void BP_SetLaserActive(bool bActive);

	/** Called when starting a dodge-roll, useful to trigger animations/trails */
	UFUNCTION(BlueprintImplementableEvent, Category = "Desperado")
	void BP_OnDodgeRollStart(FVector RollDirection);

	/** Called when finishing a dodge-roll */
	UFUNCTION(BlueprintImplementableEvent, Category = "Desperado")
	void BP_OnDodgeRollEnd();

	/** Called when throwing dynamite */
	UFUNCTION(BlueprintImplementableEvent, Category = "Desperado")
	void BP_OnDynamiteThrown();

private:
	// --- Internal State Machine Logic ---
	
	/** Checks distance to player and decides whether to transition states */
	void ProcessAIStateCheck();

	/** Enters the aiming state, pausing movement and initializing laser sight */
	void EnterAimingState();

	/** Fire the sniper shot at the player's last locked-on position */
	void FireSniperShot();

	/** Tosses dynamite at the player's feet with a parabolic velocity */
	void TossDynamite();

	/** Begins a dodge roll in a given direction */
	void StartDodgeRoll(const FVector& DodgeDirection);

	/** Ends the dodge roll, restoring movement properties */
	void EndDodgeRoll();

	/** Resets the sniper cooldown */
	void ResetSniperCooldown() { bSniperOnCooldown = false; }

	/** Resets the dynamite cooldown */
	void ResetDynamiteCooldown() { bDynamiteOnCooldown = false; }

	/** Resets the dodge cooldown */
	void ResetDodgeCooldown() { bDodgeOnCooldown = false; }

	// --- State Variables ---

	/** Current state of the Desperado */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Desperado|State", meta = (AllowPrivateAccess = "true"))
	EDesperadoState CurrentState = EDesperadoState::Idle;

	/** Cached target player reference */
	UPROPERTY()
	ACharacter* TargetPlayer = nullptr;

	/** Target position we are aiming the laser at */
	FVector AimTargetLocation;

	/** Current aim duration tracker */
	float CurrentAimTime = 0.0f;

	/** Normal walking speed cached from Movement Component */
	float OriginalMaxWalkSpeed = 200.0f;

	// Cooldown flags
	bool bSniperOnCooldown = false;
	bool bDynamiteOnCooldown = false;
	bool bDodgeOnCooldown = false;

	// Timer Handles
	FTimerHandle AICheckTimerHandle;
	FTimerHandle StateTimerHandle;
	FTimerHandle SniperCooldownTimerHandle;
	FTimerHandle DynamiteCooldownTimerHandle;
	FTimerHandle DodgeCooldownTimerHandle;
};
