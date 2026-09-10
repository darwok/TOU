// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "TwinStickCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
struct FInputActionValue;
class APlayerController;
class UInputAction;
class ATwinStickAoEAttack;
class ATwinStickProjectile;
class UActorPool;

UENUM(BlueprintType)
enum class EWeaponMode : uint8
{
	Standard,
	Shotgun,    // S-Gun (Triple Shot)
	MachineGun, // M-Gun (Rapid Fire)
	Laser       // L-Gun (Raycast Laser)
};

/**
 *  A player-controlled character for a Twin Stick Shooter game
 *  Automatically rotates to face the aim direction.
 *  Fires projectiles and spawns AoE attacks.
 */
UCLASS(abstract)
class ATwinStickCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom spring arm */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* SpringArm;

	/** Player Camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera;

	/** Projectile actor pool */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UActorPool* ProjectilePool;

protected:

	/** Jump input action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	/** Movement input action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Gamepad aim input action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* StickAimAction;

	/** Mouse aim input action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseAimAction;

	/** Dash input action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* DashAction;

	/** Shooting input action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ShootAction;

	/** AoE attack input action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AoEAction;

	/** Look input action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Lasso input action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LassoAction;

	/** Trace channel to use for mouse aim */
	UPROPERTY(EditAnywhere, Category="Input")
	TEnumAsByte<ETraceTypeQuery> MouseAimTraceChannel;

	/** Impulse to apply to the character when dashing */
	UPROPERTY(EditAnywhere, Category="Dash", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm/s"))
	float DashImpulse = 2500.0f;

	/** Type of projectile to spawn when shooting */
	UPROPERTY(EditAnywhere, Category="Projectile")
	TSubclassOf<ATwinStickProjectile> ProjectileClass;

	/** Distance ahead of the character that the projectile will be spawned at */
	UPROPERTY(EditAnywhere, Category="Projectile", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float ProjectileOffset = 100.0f;

	/** Type of AoE attack actor to spawn */
	UPROPERTY(EditAnywhere, Category="AoE")
	TSubclassOf<ATwinStickAoEAttack> AoEAttackClass;

	/** Number of starting AoE attack items */
	UPROPERTY(EditAnywhere, Category="AoE")
	int32 Items = 1;

	/** Knockback impulse to apply to the character when they're damaged */
	UPROPERTY(EditAnywhere, Category="Damage", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float KnockbackStrength = 2500.0f;

	/** Time to disallow AoE attacks after one is performed */
	UPROPERTY(EditAnywhere, Category="AoE", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float AoECooldownTime = 1.0f;

	/** Speed to blend between our current rotation and the target aim rotation when stick aiming */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 100, Units = "s"))
	float AimRotationInterpSpeed = 10.0f;

	/** Game time of the last AoE attack */
	float LastAoETime = 0.0f;

	/** Aim Yaw Angle in degrees */
	float AimAngle = 0.0f;

	/** Pointer to the player controller assigned to this character */
	TObjectPtr<APlayerController> PlayerController;

	/** If true, the player is using mouse aim */
	bool bUsingMouse = false;

	/** If true, the player is shooting */
	bool bIsShooting = false;

	/** Last time the player shot a projectile */
	float LastFireTime = 0.0f;

	/** Last held move input */
	FVector2D LastMoveInput;

	/** If true, the player is auto firing while stick aiming */
	bool bAutoFireActive = false;

	/** Time to wait between autofire attempts */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float AutoFireDelay = 0.2f;

	/** Timer to handle stick autofire */
	FTimerHandle AutoFireTimer;

	// --- Lasso Properties ---
	UPROPERTY(EditAnywhere, Category="Lasso", meta = (ClampMin = 100))
	float LassoRange = 1000.0f;

	UPROPERTY(EditAnywhere, Category="Lasso", meta = (ClampMin = 0.5))
	float LassoCooldown = 4.0f;

	UPROPERTY(EditAnywhere, Category="Lasso", meta = (ClampMin = 0.1, ClampMax = 5.0))
	float LassoStunDuration = 2.5f;

	bool bLassoOnCooldown = false;
	FTimerHandle LassoCooldownTimerHandle;

	// --- Dash Invulnerability ---
	UPROPERTY(EditAnywhere, Category="Dash", meta = (ClampMin = 0.1, ClampMax = 2.0))
	float DashDuration = 0.35f;

	bool bIsDashing = false;
	FTimerHandle DashTimerHandle;

	// --- Weapon Mode Properties ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapons")
	EWeaponMode CurrentWeaponMode = EWeaponMode::Standard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapons")
	int32 WeaponAmmo = 0;

public:
	
	/** Constructor */
	ATwinStickCharacter();

protected:

	/** Gameplay Initialization */
	virtual void BeginPlay() override;

	/** Gameplay cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Possessed by controller initialization */
	virtual void NotifyControllerChanged() override;

public:	
	
	/** Updates the character's rotation to face the aim direction */
	virtual void Tick(float DeltaTime) override;

	/** Adds input bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Handles movement inputs */
	void Move(const FInputActionValue& Value);

	/** Handles joypad aim */
	void StickAim(const FInputActionValue& Value);

	/** Handles mouse aim */
	void MouseAim(const FInputActionValue& Value);

	/** Performs a dash */
	void Dash(const FInputActionValue& Value);

	/** Handles looking around */
	void Look(const FInputActionValue& Value);

	/** Shoots projectiles */
	void Shoot(const FInputActionValue& Value);

	/** Stops shooting */
	void EndShoot(const FInputActionValue& Value);

	/** Performs an AoE Attack */
	void AoEAttack(const FInputActionValue& Value);

public:

	/** Handles move inputs from both input actions and touch interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoMove(float AxisX, float AxisY);

	/** Handles aim inputs from both input actions and touch interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoAim(float AxisX, float AxisY);

	/** Handles dash inputs from both input actions and touch interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoDash();

	// --- Wild Guns Health System ---
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Health")
	int32 Lives = 3;

	/** Evento que le avisa al HUD que reste un ícono de vida y cambie la cara del avatar */
	UFUNCTION(BlueprintImplementableEvent, Category = "Health")
	void BP_OnLifeLost(int32 RemainingLives);

	/** Evento que detona la pantalla de Game Over */
	UFUNCTION(BlueprintImplementableEvent, Category = "Health")
	void BP_OnGameOver();

	/** Handles shoot inputs from both input actions and touch interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoShoot();

	/** Handles aoe attack inputs from both input actions and touch interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoAoEAttack();

	/** Handles lasso input from both input actions and touch interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoLasso();

public:

	/** Applies collision impact to the player */
	void HandleDamage(float Damage, const FVector& DamageDirection);

protected:

	/** Allows Blueprint code to react to damage */
	UFUNCTION(BlueprintImplementableEvent, Category="Damage", meta = (DisplayName = "Damaged"))
	void BP_Damaged();

public:

	/** Gives the player a pickup item */
	void AddPickup();

	/** Upgrades the player's weapon mode with ammo */
	UFUNCTION(BlueprintCallable, Category="Weapons")
	void UpgradeWeapon(EWeaponMode NewMode, int32 InitialAmmo);

protected:

	/** Event triggered when the player throws the lasso */
	UFUNCTION(BlueprintImplementableEvent, Category="Lasso")
	void BP_OnLassoThrown(FVector Start, FVector End, bool bHitEnemy);

	/** Callback when Lasso input action is triggered */
	void Lasso(const FInputActionValue& Value);

	/** Resets Lasso cooldown */
	void ResetLassoCooldown();

	/** Callback when Dash duration timer expires to remove invulnerability */
	void EndDash();

	/** Updates the items counter on the Game Mode */
	void UpdateItems();

	/** Resets stick the aim autofire flag after the autofire timer has expired */
	void ResetAutoFire();

protected:
	/** Decal Material to use for Laser shots */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decals")
	TObjectPtr<UMaterialInterface> LaserDecalMaterial;

	/** Size of the Laser decal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decals")
	FVector LaserDecalSize = FVector(128.0f, 32.0f, 32.0f);

	/** Lifespan of the Laser decal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decals")
	float LaserDecalLifeSpan = 5.0f;

	/** Event triggered when player shoots laser */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapons")
	void BP_OnLaserShot(FVector Start, FVector End, bool bHit);
};
