// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DecalActor.h"
#include "DecalImpactBullet.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_URO_API ADecalImpactBullet : public ADecalActor
{
	GENERATED_BODY()

public:
	ADecalImpactBullet();


	UFUNCTION(BlueprintCallable, Category = "Decal")
	void SetupDecal(const FHitResult& hit, float lifeSpan = 2, float fadeTime = 1);

protected:
	
	FTimerHandle timerHandle;
	void ReturnDecalToPool();
};
