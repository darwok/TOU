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
#include "Components/PrimitiveComponent.h"

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

	if (PlayerController)
	{
		FVector MouseLocation, MouseDirection;
		if (PlayerController->DeprojectMousePositionToWorld(MouseLocation, MouseDirection))
		{
			// El cuerpo persigue el mismo punto infinito que las balas
			FVector TargetPoint = MouseLocation + (MouseDirection * 50000.0f);

			FVector LookDirection = TargetPoint - GetActorLocation();
			LookDirection.Z = 0.0f;

			if (!LookDirection.IsNearlyZero())
			{
				FRotator TargetRot = LookDirection.Rotation();
				FRotator SmoothRot = FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, AimRotationInterpSpeed);
				SetActorRotation(SmoothRot);
			}
		}
	}
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

	// Sumamos ambos ejes para capturar el input lateral sin importar el mapeo de teclas
	float LateralInput = AxisX + AxisY;

	LastMoveInput.X = 0.0f;
	LastMoveInput.Y = LateralInput;

	FRotator FlatRot = GetControlRotation();
	FlatRot.Pitch = 0.0f;
	FlatRot.Roll = 0.0f;

	// Aplicamos el input únicamente sobre el vector derecho (Strafe)
	AddMovementInput(FlatRot.RotateVector(FVector::RightVector), LateralInput);
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
	if (!ProjectilePool) return;
	if (GetCharacterMovement()->IsFalling()) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	float FireDelay = (CurrentWeaponMode == EWeaponMode::MachineGun) ? 0.08f : (CurrentWeaponMode == EWeaponMode::Laser ? 0.15f : 0.22f);
	if (CurrentTime - LastFireTime < FireDelay) return;
	LastFireTime = CurrentTime;
	// 1. Proyectar el objetivo a 500 metros en la dirección de la cámara (Crosshair)
	FVector TargetPoint = GetActorLocation() + (GetActorForwardVector() * 50000.0f);
	if (PlayerController)
	{
		FVector MouseLocation, MouseDirection;
		if (PlayerController->DeprojectMousePositionToWorld(MouseLocation, MouseDirection))
		{
			TargetPoint = MouseLocation + (MouseDirection * 50000.0f);
		}
	}

	// 2. SPAWN SEGURO: La bala siempre nace estrictamente hacia el frente del jugador (Como en el proyecto original)
	FVector ChestLocation = GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
	FVector ProjectileLocation = ChestLocation + (GetActorForwardVector() * 100.0f);

	// 3. APUNTADO: Calculamos el ángulo desde esa posición segura hacia el punto del mouse
	FVector TrueAimDirection = (TargetPoint - ProjectileLocation).GetSafeNormal();

	FTransform ProjectileTransform = GetActorTransform();
	ProjectileTransform.SetLocation(ProjectileLocation);
	ProjectileTransform.SetRotation(TrueAimDirection.Rotation().Quaternion());

	// 3. Disparo de Armas
	if (CurrentWeaponMode == EWeaponMode::Laser)
	{
		FVector StartLocation = ProjectileLocation;
		FVector EndLocation = StartLocation + TrueAimDirection * 2000.0f;

		FHitResult HitResult;
		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(this);

		bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, TraceParams);
		BP_OnLaserShot(StartLocation, bHit ? HitResult.ImpactPoint : EndLocation, bHit);

		if (bHit)
		{
			if (ATwinStickNPC* NPC = Cast<ATwinStickNPC>(HitResult.GetActor()))
			{
				NPC->ProjectileImpact(TrueAimDirection);
			}
		}
		WeaponAmmo--;
	}
	else if (CurrentWeaponMode == EWeaponMode::Shotgun)
	{
		float ConeHalfAngle = FMath::DegreesToRadians(4.0f);
		for (int32 i = 0; i < 5; ++i)
		{
			FVector RandomDir = FMath::VRandCone(TrueAimDirection, ConeHalfAngle);
			AActor* PooledActor = ProjectilePool->GetActorFromPool(ProjectileLocation, RandomDir.Rotation());

			if (ATwinStickProjectile* Projectile = Cast<ATwinStickProjectile>(PooledActor))
			{
				Projectile->OwningPool = ProjectilePool;
				Projectile->SetOwner(this);
				Projectile->SetInstigator(this);

				// La bala ignora físicamente al jugador al moverse
				if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Projectile->GetRootComponent()))
				{
					RootPrim->IgnoreActorWhenMoving(this, true);
				}
			}
		}
		WeaponAmmo--;
	}
	else
	{
		// Standard y MachineGun
		AActor* PooledActor = ProjectilePool->GetActorFromPool(ProjectileLocation, ProjectileTransform.Rotator());
		if (ATwinStickProjectile* Projectile = Cast<ATwinStickProjectile>(PooledActor))
		{
			Projectile->OwningPool = ProjectilePool;
			Projectile->SetOwner(this);
			Projectile->SetInstigator(this);

			// La bala ignora físicamente al jugador al moverse
			if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Projectile->GetRootComponent()))
			{
				RootPrim->IgnoreActorWhenMoving(this, true);
			}
		}

		if (CurrentWeaponMode == EWeaponMode::MachineGun)
		{
			WeaponAmmo--;
		}
	}

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

			// --- NUEVA LÓGICA DE LANZAMIENTO ---

			// 1. Obtener la ubicación inicial (con un offset hacia adelante para que no choque contigo)
			FVector SpawnLocation = GetActorLocation() + (GetActorForwardVector() * 100.0f);

			// 2. Calcular hacia dónde apunta el mouse
			FRotator SpawnRotation = GetActorRotation(); // Rotación por defecto por si falla el mouse

			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				FHitResult HitResult;
				if (PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
				{
					// Crear una rotación que mire desde el jugador hacia el cursor
					FVector Direction = HitResult.Location - SpawnLocation;
					SpawnRotation = Direction.Rotation();
				}
			}

			// spawn the AoE (Ahora usa la nueva ubicación y rotación calculadas)
			AActor* AoE = GetWorld()->SpawnActor<AActor>(AoEAttackClass, SpawnLocation, SpawnRotation);

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

