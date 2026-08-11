// Fill out your copyright notice in the Description page of Project Settings.

#include "ActorPool.h"
#include "ActorUtilities.h"

// Sets default values for this component's properties
UActorPool::UActorPool()
{
	// Desactivamos el tick de este componente para optimizar rendimiento
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void UActorPool::BeginPlay()
{
	Super::BeginPlay();

	if (!actorTemplate) 
	{
		UE_LOG(LogTemp, Warning, TEXT("ActorPool en %s no tiene asignado un actorTemplate."), *GetOwner()->GetName());
		return;
	}

	if (defaultSize <= 0) 
	{
		return;
	}

	// Pre-instanciar los actores del pool
	for (int32 i = 0; i < defaultSize; i++) 
	{
		InstancePoolActor();
	}
}

AActor* UActorPool::InstancePoolActor()
{
	if (!actorTemplate)
	{
		return nullptr;
	}

	FActorSpawnParameters spawnInfo;
	spawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	spawnInfo.Owner = GetOwner();

	AActor* NewActor = GetWorld()->SpawnActor<AActor>(actorTemplate, spawnInfo);

	if (!NewActor) 
	{
		UE_LOG(LogTemp, Error, TEXT("No se pudo instanciar el actor para el pool en %s."), *GetOwner()->GetName());
		return nullptr;
	}

	// Agregar al pool
	actorPool.Add(NewActor);
	
	// Desactivar el actor inicialmente
	UActorUtilities::ToggleActorHidden(NewActor, true);

	return NewActor;
}

AActor* UActorPool::FindFirstAvailableActor()
{
	// 1. Limpieza preventiva de punteros nulos (por si algún actor fue destruido externamente)
	actorPool.RemoveAll([](AActor* Actor) {
		return !IsValid(Actor);
	});

	// 2. Buscar primer actor disponible (inactivo)
	for (AActor* Actor : actorPool) 
	{
		if (Actor && Actor->IsHidden()) 
		{
			return Actor;
		}
	}

	// 3. Si no hay disponibles, crear uno nuevo dinámicamente (crecimiento bajo demanda)
	return InstancePoolActor();
}

AActor* UActorPool::GetActorFromPool(const FVector& Location, const FRotator& Rotation)
{
	AActor* ActorFound = FindFirstAvailableActor();

	if (ActorFound)
	{
		// Teletransportar físicamente reseteando velocidades para evitar físicas raras de arrastre
		ActorFound->SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
		
		// Activar el actor
		UActorUtilities::ToggleActorHidden(ActorFound, false);
		return ActorFound;
	}

	return nullptr;
}

AActor* UActorPool::GetActorFromPoolDefault()
{
	AActor* ActorFound = FindFirstAvailableActor();

	if (ActorFound)
	{
		// Activar el actor sin alterar su transformación previa
		UActorUtilities::ToggleActorHidden(ActorFound, false);
		return ActorFound;
	}

	return nullptr;
}

void UActorPool::ReturnActorToPool(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	// Asegurarse de que pertenece a este pool
	if (actorPool.Contains(Actor))
	{
		UActorUtilities::ToggleActorHidden(Actor, true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Se intentó devolver al pool un actor (%s) que no pertenece a él."), *Actor->GetName());
	}
}
