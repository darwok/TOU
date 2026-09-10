// Copyright Epic Games, Inc. All Rights Reserved.


#include "TwinStickCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "TwinStickGameMode.h"
#include "TwinStickAoEAttack.h"
#include "Kismet/KismetMathLibrary.h"
#include "TwinStickProjectile.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Pooling/ActorPool.h"
#include "AI/TwinStickNPC.h"
#include "Pooling/PooledDecalActor.h"

ATwinStickCharacter::ATwinStickCharacter()
{
 	PrimaryActorTick.bCanEverTick = true;

	// create the spring arm
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	SpringArm->SetupAttachment(RootComponent);

	SpringArm->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f));

	SpringArm->TargetArmLength = 400.0f;
	SpringArm->bDoCollisionTest = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritRoll = true;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 3.0f;

	// create the camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	Camera->SetFieldOfView(75.0f);
	Camera->bUsePawnControlRotation = false;

	// create the projectile pool
	ProjectilePool = CreateDefaultSubobject<UActorPool>(TEXT("ProjectilePool"));
	ProjectilePool->defaultSize = 20;

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// configure the character movement
	GetCharacterMovement()->GravityScale = 1.5f;
	GetCharacterMovement()->MaxAcceleration = 1000.0f;
	GetCharacterMovement()->BrakingFrictionFactor = 1.0f;
	GetCharacterMovement()->bCanWalkOffLedges = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
	GetCharacterMovement()->bConstrainToPlane = false;
	GetCharacterMovement()->bSnapToPlaneAtStart = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// Habilitar doble salto para mecánicas de evasión
	JumpMaxCount = 2;
}

void ATwinStickCharacter::BeginPlay()
{
	// Initialize projectile pool template
	if (ProjectilePool && ProjectileClass)
	{
		ProjectilePool->actorTemplate = ProjectileClass;
	}

	Super::BeginPlay();

	// update the items count
	UpdateItems();

	//// Hide mouse cursor and capture input for 3rd person camera look controls
	//if (PlayerController)
	//{
	//	PlayerController->SetShowMouseCursor(false);
	//	FInputModeGameOnly InputMode;
	//	PlayerController->SetInputMode(InputMode);
	//}
	// Mostrar el cursor y permitir input mixto (Wild Guns)
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(true);
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}
}

void ATwinStickCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	/** Clear the autofire timer */
	GetWorld()->GetTimerManager().ClearTimer(AutoFireTimer);
}

void ATwinStickCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// set the player controller reference
	PlayerController = Cast<APlayerController>(GetController());
}

void ATwinStickCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// In Third Person, we smoothly rotate to face the camera direction if shooting or aiming
	/*if ((bIsShooting || bAutoFireActive) && PlayerController)
	{
		FRotator ControlRot = GetControlRotation();
		ControlRot.Pitch = 0.0f;
		ControlRot.Roll = 0.0f;
		
		FRotator TargetRot = FMath::RInterpTo(GetActorRotation(), ControlRot, DeltaTime, AimRotationInterpSpeed);
		SetActorRotation(TargetRot);
	}*/
}

void ATwinStickCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// set up the enhanced input action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATwinStickCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATwinStickCharacter::Look);
		EnhancedInputComponent->BindAction(StickAimAction, ETriggerEvent::Triggered, this, &ATwinStickCharacter::StickAim);
		EnhancedInputComponent->BindAction(MouseAimAction, ETriggerEvent::Triggered, this, &ATwinStickCharacter::MouseAim);
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Triggered, this, &ATwinStickCharacter::Dash);
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Triggered, this, &ATwinStickCharacter::Shoot);
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Completed, this, &ATwinStickCharacter::EndShoot);
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Canceled, this, &ATwinStickCharacter::EndShoot);
		EnhancedInputComponent->BindAction(AoEAction, ETriggerEvent::Triggered, this, &ATwinStickCharacter::AoEAttack);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		if (LassoAction)
		{
			EnhancedInputComponent->BindAction(LassoAction, ETriggerEvent::Triggered, this, &ATwinStickCharacter::Lasso);
		}
	}
}

void ATwinStickCharacter::Move(const FInputActionValue& Value)
{
	// save the input vector
	FVector2D InputVector = Value.Get<FVector2D>();

	// route the input
	DoMove(InputVector.X, InputVector.Y);
}

void ATwinStickCharacter::StickAim(const FInputActionValue& Value)
{
	// get the input vector
	FVector2D InputVector = Value.Get<FVector2D>();

	// route the input
	DoAim(InputVector.X, InputVector.Y);
}

void ATwinStickCharacter::MouseAim(const FInputActionValue& Value)
{
	// In third person, we don't show the mouse cursor
	bUsingMouse = true;
}

void ATwinStickCharacter::Dash(const FInputActionValue& Value)
{
	// route the input
	DoDash();
}

void ATwinStickCharacter::Look(const FInputActionValue& Value)
{
	// get the input vector
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ATwinStickCharacter::Shoot(const FInputActionValue& Value)
{
	bIsShooting = true;
	// route the input
	DoShoot();
}

void ATwinStickCharacter::EndShoot(const FInputActionValue& Value)
{
	bIsShooting = false;
}

void ATwinStickCharacter::AoEAttack(const FInputActionValue& Value)
{
	// route the input
	DoAoEAttack();
}

void ATwinStickCharacter::DoMove(float AxisX, float AxisY)
{
	// Regla Wild Guns: No se puede mover mientras se dispara
	if (bIsShooting)
	{
		return;
	}

	// Anulamos la profundidad (AxisX en este Input) y conservamos el lateral (AxisY)
	LastMoveInput.X = 0.0f;
	LastMoveInput.Y = AxisY;

	FRotator FlatRot = GetControlRotation();
	FlatRot.Pitch = 0.0f;
	FlatRot.Roll = 0.0f;

	// Aplicamos ÚNICAMENTE el input lateral (Strafe) usando AxisY
	AddMovementInput(FlatRot.RotateVector(FVector::RightVector), AxisY);
}

void ATwinStickCharacter::DoAim(float AxisX, float AxisY)
{
	// lower the mouse controls flag
	bUsingMouse = false;

	// hide the mouse cursor
	if (PlayerController)
	{
		/*PlayerController->SetShowMouseCursor(false);*/
	}

	// are we on autofire cooldown?
	if (!bAutoFireActive)
	{
		// set ourselves on cooldown
		bAutoFireActive = true;

		// fire a projectile
		DoShoot();

		// schedule autofire cooldown reset (Machine Gun and Laser have custom auto fire rates)
		float CurrentDelay = AutoFireDelay;
		if (CurrentWeaponMode == EWeaponMode::MachineGun)
		{
			CurrentDelay = 0.08f;
		}
		else if (CurrentWeaponMode == EWeaponMode::Laser)
		{
			CurrentDelay = 0.15f;
		}
		GetWorld()->GetTimerManager().SetTimer(AutoFireTimer, this, &ATwinStickCharacter::ResetAutoFire, CurrentDelay, false);
	}
}

void ATwinStickCharacter::DoDash()
{
	// calculate the launch impulse vector based on the last move input
	FVector LaunchDir = FVector::ZeroVector;

	LaunchDir.X = FMath::Clamp(LastMoveInput.X, -1.0f, 1.0f);
	LaunchDir.Y = FMath::Clamp(LastMoveInput.Y, -1.0f, 1.0f);

	// launch the character in the chosen direction
	LaunchCharacter(LaunchDir * DashImpulse, true, true);

	// Dash invulnerability
	bIsDashing = true;
	GetWorld()->GetTimerManager().SetTimer(DashTimerHandle, this, &ATwinStickCharacter::EndDash, DashDuration, false);
}

void ATwinStickCharacter::DoShoot()
{
	// Regla Wild Guns: No se puede disparar si se está saltando
	if (GetCharacterMovement()->IsFalling())
	{
		return;
	}

	if (!ProjectilePool)
	{
		return;
	}

	// Enforce fire rate cooldown
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float FireDelay = 0.22f;
	if (CurrentWeaponMode == EWeaponMode::MachineGun)
	{
		FireDelay = 0.08f;
	}
	else if (CurrentWeaponMode == EWeaponMode::Laser)
	{
		FireDelay = 0.15f;
	}

	if (CurrentTime - LastFireTime < FireDelay)
	{
		return;
	}
	LastFireTime = CurrentTime;

	//// Obtenemos la posición inicial de la bala (offset desde el jugador)
	//FTransform ProjectileTransform = GetActorTransform();
	//FVector ProjectileLocation = ProjectileTransform.GetLocation() + (GetActorForwardVector() * ProjectileOffset);
	//ProjectileTransform.SetLocation(ProjectileLocation);

	//// Regla Wild Guns (Opción A): Apuntar hacia la posición del Mouse en 3D
	//if (PlayerController)
	//{
	//	FHitResult HitResult;
	//	// Lanzamos un rayo invisible desde la pantalla hacia el mundo 3D
	//	bool bHit = PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

	//	if (bHit)
	//	{
	//		// Si el cursor toca el fondo/enemigo, calculamos el ángulo desde el jugador hacia ese punto
	//		FRotator AimRotation = (HitResult.ImpactPoint - ProjectileLocation).Rotation();
	//		ProjectileTransform.SetRotation(AimRotation.Quaternion());
	//	}
	//	else
	//	{
	//		// Si el cursor está en el cielo/fuera del nivel, disparamos recto por defecto
	//		ProjectileTransform.SetRotation(GetActorRotation().Quaternion());
	//	}
	//}
	FTransform ProjectileTransform = GetActorTransform();
	FVector ProjectileLocation = ProjectileTransform.GetLocation() + (GetActorForwardVector() * ProjectileOffset);
	ProjectileTransform.SetLocation(ProjectileLocation);

	// Apuntado Wild Guns: Rayo de cámara a fondo ignorando la espalda del jugador
	if (PlayerController)
	{
		FVector MouseLocation, MouseDirection;
		if (PlayerController->DeprojectMousePositionToWorld(MouseLocation, MouseDirection))
		{
			// Trazamos 500 metros hacia el fondo
			FVector TraceEnd = MouseLocation + (MouseDirection * 50000.0f);
			FHitResult HitResult;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this); // Ignorar explícitamente al jugador

			bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, MouseLocation, TraceEnd, ECC_Visibility, QueryParams);

			if (bHit)
			{
				// Si toca una pared o enemigo, dispara hacia ese impacto
				FRotator AimRotation = (HitResult.ImpactPoint - ProjectileLocation).Rotation();
				ProjectileTransform.SetRotation(AimRotation.Quaternion());
			}
			else
			{
				// Si apuntas al cielo vacío, dispara en esa trayectoria
				FRotator AimRotation = (TraceEnd - ProjectileLocation).Rotation();
				ProjectileTransform.SetRotation(AimRotation.Quaternion());
			}
		}
	}

	if (CurrentWeaponMode == EWeaponMode::Laser)
	{
		// Raycast Laser shoot
		FVector StartLocation = ProjectileLocation;
		FVector Direction = ProjectileTransform.GetRotation().GetForwardVector();
		FVector EndLocation = StartLocation + Direction * 2000.0f; // 2000 cm range

		FHitResult HitResult;
		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(this);

		bool bHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			StartLocation,
			EndLocation,
			ECC_Visibility,
			TraceParams
		);

		// Trigger Blueprint event for laser beam representation
		BP_OnLaserShot(StartLocation, bHit ? HitResult.ImpactPoint : EndLocation, bHit);

		if (bHit)
		{
			if (!LaserDecalMaterial)
			{
				UE_LOG(LogTemp, Warning, TEXT("TwinStickCharacter: LaserDecalMaterial no está asignado en el Blueprint."));
			}

			// If we hit an NPC, apply damage
			if (ATwinStickNPC* NPC = Cast<ATwinStickNPC>(HitResult.GetActor()))
			{
				NPC->ProjectileImpact(Direction);
			}

			// Spawn impact decal from pool
			if (ATwinStickGameMode* GM = Cast<ATwinStickGameMode>(GetWorld()->GetAuthGameMode()))
			{
				if (UActorPool* DecalPool = GM->GetDecalPool())
				{
					// Orient projection INTO the wall surface using (-HitResult.ImpactNormal)
					FRotator DecalRotation = (-HitResult.ImpactNormal).Rotation();
					DecalRotation.Roll = FMath::RandRange(0.0f, 360.0f);

					AActor* PooledActor = DecalPool->GetActorFromPool(HitResult.ImpactPoint, DecalRotation);
					if (APooledDecalActor* DecalActor = Cast<APooledDecalActor>(PooledActor))
					{
						DecalActor->OwningPool = DecalPool;
						DecalActor->InitDecal(LaserDecalMaterial, LaserDecalSize, LaserDecalLifeSpan);
					}
				}
			}
		}

		WeaponAmmo--;
	}
	else if (CurrentWeaponMode == EWeaponMode::Shotgun)
	{
		// S-Gun: Triple spread shot
		FRotator BaseRot = ProjectileTransform.Rotator();
		float SpreadAngle = 15.0f;

		float Angles[3] = { -SpreadAngle, 0.0f, SpreadAngle };
		for (int32 i = 0; i < 3; ++i)
		{
			FRotator BulletRot = BaseRot;
			BulletRot.Yaw += Angles[i];

			AActor* PooledActor = ProjectilePool->GetActorFromPool(ProjectileLocation, BulletRot);
			if (ATwinStickProjectile* Projectile = Cast<ATwinStickProjectile>(PooledActor))
			{
				Projectile->OwningPool = ProjectilePool;
			}
		}

		WeaponAmmo--;
	}
	else
	{
		// Standard or MachineGun: Single shot
		AActor* PooledActor = ProjectilePool->GetActorFromPool(ProjectileLocation, ProjectileTransform.Rotator());
		if (ATwinStickProjectile* Projectile = Cast<ATwinStickProjectile>(PooledActor))
		{
			Projectile->OwningPool = ProjectilePool;
		}

		if (CurrentWeaponMode == EWeaponMode::MachineGun)
		{
			WeaponAmmo--;
		}
	}

	// Return to Standard weapon if upgraded ammo is depleted
	if (CurrentWeaponMode != EWeaponMode::Standard && WeaponAmmo <= 0)
	{
		CurrentWeaponMode = EWeaponMode::Standard;
	}
}

void ATwinStickCharacter::DoAoEAttack()
{
	// do we have enough items to do an AoE attack?
	if (Items > 0)
	{
		// get the game time
		const float GameTime = GetWorld()->GetTimeSeconds();

		// are we off AoE cooldown?
		if (GameTime - LastAoETime > AoECooldownTime)
		{
			// save the new AoE time
			LastAoETime = GameTime;

			// spawn the AoE
			ATwinStickAoEAttack* AoE = GetWorld()->SpawnActor<ATwinStickAoEAttack>(AoEAttackClass, GetActorTransform());

			// decrease the number of items
			--Items;

			// update the items count
			UpdateItems();
		}
	}
}

void ATwinStickCharacter::HandleDamage(float Damage, const FVector& DamageDirection)
{
	// Si está haciendo Dash (esquivando), es invulnerable
	if (bIsDashing)
	{
		return;
	}

	// Regla Wild Guns: 1 golpe quita exactamente 1 vida completa (1 HP)
	Lives--;

	if (Lives > 0)
	{
		// Aún hay vidas: Aplicar knockback para dar feedback
		FVector LaunchVector = DamageDirection;
		LaunchVector.Z = 0.0f;
		LaunchCharacter(LaunchVector * KnockbackStrength, true, true);

		// Disparamos los eventos visuales hacia los Blueprints / HUD
		BP_Damaged();
		BP_OnLifeLost(Lives);
	}
	else
	{
		// Vidas en 0: El jugador ha sido derrotado
		BP_OnGameOver();
	}
}

void ATwinStickCharacter::AddPickup()
{
	// increase the item count
	++Items;

	// update the items counter
	UpdateItems();
}

void ATwinStickCharacter::UpgradeWeapon(EWeaponMode NewMode, int32 InitialAmmo)
{
	CurrentWeaponMode = NewMode;
	WeaponAmmo = InitialAmmo;
}

void ATwinStickCharacter::Lasso(const FInputActionValue& Value)
{
	DoLasso();
}

void ATwinStickCharacter::DoLasso()
{
	if (bLassoOnCooldown)
	{
		return;
	}

	FVector StartLoc = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	FVector EndLoc = StartLoc + GetActorForwardVector() * LassoRange;

	FHitResult HitResult;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);

	// Trace to see if we hit an NPC
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLoc,
		EndLoc,
		ECC_Pawn,
		TraceParams
	);

	bool bStunnedEnemy = false;
	if (bHit && HitResult.GetActor())
	{
		if (ATwinStickNPC* NPC = Cast<ATwinStickNPC>(HitResult.GetActor()))
		{
			NPC->ApplyLassoStun(LassoStunDuration);
			bStunnedEnemy = true;
		}
	}

	// Trigger visual rope/effect in BP
	BP_OnLassoThrown(StartLoc, EndLoc, bStunnedEnemy);

	// Put lasso on cooldown
	bLassoOnCooldown = true;
	GetWorld()->GetTimerManager().SetTimer(LassoCooldownTimerHandle, this, &ATwinStickCharacter::ResetLassoCooldown, LassoCooldown, false);
}

void ATwinStickCharacter::ResetLassoCooldown()
{
	bLassoOnCooldown = false;
}

void ATwinStickCharacter::EndDash()
{
	bIsDashing = false;
}

void ATwinStickCharacter::UpdateItems()
{
	// update the game mode
	if (ATwinStickGameMode* GM = Cast<ATwinStickGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->ItemUsed(Items);
	}
}

void ATwinStickCharacter::ResetAutoFire()
{
	// reset the autofire flag
	bAutoFireActive = false;
}

