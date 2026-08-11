// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ActorUtilities.generated.h"

/**
 * Biblioteca de funciones de utilidad para la gestión de actores del Pool.
 */
UCLASS()
class PROJECT_URO_API UActorUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/**
	 * Activa o desactiva un actor por completo (visibilidad, tick, colisión, física y componentes de movimiento).
	 * También notifica al actor si implementa la interfaz IPoolableActor.
	 * 
	 * @param ActorEl actor a modificar.
	 * @param bHiddenTrue para desactivar y devolver al pool, False para activar y usar.
	 */
	UFUNCTION(BlueprintCallable, Category = "Actor Utilities")
	static void ToggleActorHidden(AActor* Actor, bool bHidden);
};
