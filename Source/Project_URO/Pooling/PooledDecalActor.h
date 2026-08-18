// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pooling/PoolableActor.h"
#include "PooledDecalActor.generated.h"

class UDecalComponent;
class UMaterialInterface;

/**
 * Un actor de decal que se puede almacenar en el ActorPool.
 * Permite renderizar decals de colisión de forma optimizada y reutilizarlos.
 */
UCLASS()
class PROJECT_URO_API APooledDecalActor : public AActor, public IPoolableActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APooledDecalActor();

	/** Componente de Decal para pintar la textura en el escenario. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UDecalComponent* DecalComponent;

	/** Tiempo por defecto que permanece el decal antes de volver al pool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal")
	float LifeSpan = 5.0f;

	/** Tamaño por defecto del decal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal")
	FVector DecalSize = FVector(128.0f, 32.0f, 32.0f);

	/** Inicializa el decal con su material y escala antes de activarse. */
	UFUNCTION(BlueprintCallable, Category = "Decal")
	void InitDecal(UMaterialInterface* Material, const FVector& Size, float LifeSpanTime);

	/** IPoolableActor overrides */
	virtual void OnActivatedFromPool_Implementation() override;
	virtual void OnReturnedToPool_Implementation() override;

	/** Pool al que pertenece este decal para poder regresar de forma segura. */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Pooling")
	class UActorPool* OwningPool;

	/** Devuelve este decal de vuelta al pool. */
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void ReturnToPool();

protected:
	FTimerHandle LifeSpanTimerHandle;
};
