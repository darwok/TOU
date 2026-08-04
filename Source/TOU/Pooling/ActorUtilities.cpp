// Fill out your copyright notice in the Description page of Project Settings.

#include "ActorUtilities.h"
#include "PoolableActor.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/MovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraComponent.h"

void UActorUtilities::ToggleActorHidden(AActor* Actor, bool bHidden)
{
	if (!Actor) 
	{
		return;
	}

	// 1. Configuración básica de visibilidad, colisión y tick a nivel de Actor
	Actor->SetActorHiddenInGame(bHidden);
	Actor->SetActorTickEnabled(!bHidden);
	Actor->SetActorEnableCollision(!bHidden);

	// 2. Gestionar componentes internos (Física, Movimiento, Partículas, Ticks)
	TInlineComponentArray<UActorComponent*> Components(Actor);
	for (UActorComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		// Desactivar/Activar tick del componente hijo
		Component->SetComponentTickEnabled(!bHidden);

		// Componentes de movimiento (ej. ProjectileMovementComponent)
		if (UMovementComponent* MovementComp = Cast<UMovementComponent>(Component))
		{
			if (bHidden)
			{
				MovementComp->StopMovementImmediately();
				MovementComp->Deactivate();
			}
			else
			{
				MovementComp->Activate(true);
			}
		}

		// Componentes físicos (desactivar simulación de física para que no sigan cayendo/moviéndose)
		if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(Component))
		{
			if (PrimitiveComp->IsSimulatingPhysics())
			{
				// Si simula física, detenemos su velocidad y opcionalmente pausamos la simulación
				PrimitiveComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
				PrimitiveComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				
				// Opcional: Desactivar simulación física mientras esté inactivo
				// PrimitiveComp->SetSimulatePhysics(!bHidden);
			}
		}

		// Componentes de partículas (detener efectos activos cuando se oculta)
		if (UParticleSystemComponent* ParticleComp = Cast<UParticleSystemComponent>(Component))
		{
			if (bHidden)
			{
				ParticleComp->DeactivateSystem();
			}
			else
			{
				ParticleComp->ActivateSystem(true);
			}
		}

		// Soporte nativo para Niagara
		if (UNiagaraComponent* NiagaraComp = Cast<UNiagaraComponent>(Component))
		{
			if (bHidden)
			{
				NiagaraComp->Deactivate();
			}
			else
			{
				NiagaraComp->Activate(true);
			}
		}
	}

	// 3. Notificar a través de la Interfaz IPoolableActor (C++ o Blueprints)
	if (Actor->GetClass()->ImplementsInterface(UPoolableActor::StaticClass()))
	{
		if (bHidden)
		{
			IPoolableActor::Execute_OnReturnedToPool(Actor);
		}
		else
		{
			IPoolableActor::Execute_OnActivatedFromPool(Actor);
		}
	}
}
