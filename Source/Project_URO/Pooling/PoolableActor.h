// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PoolableActor.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UPoolableActor : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interfaz para actores que se pueden almacenar en el ActorPool.
 * Permite responder a eventos cuando el actor entra o sale del pool.
 */
class PROJECT_URO_API IPoolableActor
{
	GENERATED_BODY()

public:
	/** Llamado cuando el actor es sacado del pool para ser usado. */
	UFUNCTION(BlueprintNativeEvent, Category = "Poolable Actor")
	void OnActivatedFromPool();

	/** Llamado cuando el actor es devuelto al pool. */
	UFUNCTION(BlueprintNativeEvent, Category = "Poolable Actor")
	void OnReturnedToPool();
};
