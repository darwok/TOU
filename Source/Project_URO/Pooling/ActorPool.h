// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActorPool.generated.h"

/**
 * Componente que gestiona un pool reutilizable de un tipo específico de actor.
 * Ayuda a evitar tirones de rendimiento (garbage collection y CPU overhead) en UE.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_URO_API UActorPool : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UActorPool();

	/** Tamaño inicial del pool. Se crearán estos actores al arrancar el juego. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Pool")
	int32 defaultSize = 10;

	/** La clase del actor que se almacenará y reutilizará en el pool. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Pool")
	TSubclassOf<AActor> actorTemplate;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/** Arreglo interno que contiene todos los actores administrados por el pool. */
	UPROPERTY()
	TArray<AActor*> actorPool;

	/** Crea una nueva instancia del actor, lo registra en el pool y lo desactiva. */
	AActor* InstancePoolActor();

	/** Busca el primer actor inactivo en el pool. Si no hay ninguno, instancia uno nuevo de forma dinámica. */
	AActor* FindFirstAvailableActor();

public:	
	/**
	 * Obtiene un actor del Pool y lo activa en la posición y rotación especificadas.
	 * Si no hay actores libres, creará uno nuevo automáticamente ampliando el pool.
	 * 
	 * @param Location Posición espacial donde ubicar al actor.
	 * @param Rotation Rotación a aplicar al actor.
	 * @return El actor activado o nullptr en caso de error.
	 */
	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	AActor* GetActorFromPool(const FVector& Location, const FRotator& Rotation);

	/**
	 * Obtiene un actor del Pool sin modificar su transformación por defecto.
	 */
	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	AActor* GetActorFromPoolDefault();

	/**
	 * Devuelve un actor de vuelta al pool de forma explícita, desactivando sus colisiones, física y ticks.
	 * 
	 * @param ActorEl actor que se desea regresar al pool.
	 */
	UFUNCTION(BlueprintCallable, Category = "Actor Pool")
	void ReturnActorToPool(AActor* Actor);
};
