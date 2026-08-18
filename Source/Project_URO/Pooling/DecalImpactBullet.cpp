// Fill out your copyright notice in the Description page of Project Settings.


#include "DecalImpactBullet.h"
#include "Components/DecalComponent.h"
#include "ActorUtilities.h"

ADecalImpactBullet::ADecalImpactBullet()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADecalImpactBullet::SetupDecal(const FHitResult& hit, float lifeSpan, float fadeTime)
{
	SetActorLocation(hit.ImpactPoint);
	SetActorRotation(hit.ImpactNormal.Rotation());

	UDecalComponent* decalComp = GetDecal();

	if (!decalComp) {
		return;
	}

	//resetar valor al salir del pool
	decalComp->SetVisibility(true);
	decalComp->FadeDuration = 0;
	decalComp->FadeInStartDelay = 0;

	decalComp->SetFadeOut(lifeSpan - fadeTime, fadeTime, false);

	GetWorldTimerManager().SetTimer(
		timerHandle, //el hanlde donde se guarda
		this,// el objeto que lo activa
		&ADecalImpactBullet::ReturnDecalToPool, // la función que se activa al terminar
		lifeSpan, //la ducarión
		false // si hace loop o no
	);

}

void ADecalImpactBullet::ReturnDecalToPool()
{
	UActorUtilities::ToggleActorHidden(this, true);
}
