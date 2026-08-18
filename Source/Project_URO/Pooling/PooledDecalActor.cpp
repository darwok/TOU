// Fill out your copyright notice in the Description page of Project Settings.


#include "PooledDecalActor.h"
#include "Components/DecalComponent.h"
#include "Pooling/ActorPool.h"
#include "TimerManager.h"
#include "Engine/World.h"

// Sets default values
APooledDecalActor::APooledDecalActor()
{
 	// Decals do not need tick
	PrimaryActorTick.bCanEverTick = false;

	// Create and attach the DecalComponent as root
	DecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComponent"));
	RootComponent = DecalComponent;

	// Set default size
	DecalComponent->DecalSize = DecalSize;
}

void APooledDecalActor::InitDecal(UMaterialInterface* Material, const FVector& Size, float LifeSpanTime)
{
	if (DecalComponent)
	{
		DecalComponent->SetDecalMaterial(Material);
		DecalComponent->DecalSize = Size;

		// Calculate relative fade parameters:
		// Fade out over the last 1.5 seconds or 30% of the lifespan, whichever is smaller.
		float FadeDuration = FMath::Min(1.5f, LifeSpanTime * 0.3f);
		float FadeDelay = FMath::Max(0.1f, LifeSpanTime - FadeDuration);
		
		// SetFadeOut automatically handles fading out the decal relative to its activation time
		// without destroying the actor itself (bDestroyOnFadeOut = false)
		DecalComponent->SetFadeOut(FadeDelay, FadeDuration, false);
	}

	LifeSpan = LifeSpanTime;

	// Set timer to return the actor to its pool when lifespan expires
	GetWorld()->GetTimerManager().SetTimer(LifeSpanTimerHandle, this, &APooledDecalActor::ReturnToPool, LifeSpan, false);
}

void APooledDecalActor::OnActivatedFromPool_Implementation()
{
	// Decal component is already activated. Any relative fade setups are handled in InitDecal.
}

void APooledDecalActor::OnReturnedToPool_Implementation()
{
	// Clear the active timer when returning to pool
	GetWorld()->GetTimerManager().ClearTimer(LifeSpanTimerHandle);
}

void APooledDecalActor::ReturnToPool()
{
	// Clear active timer
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
