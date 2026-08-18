// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TwinStickHUD.generated.h"

class UTwinStickUI;

/**
 * HUD principal para el juego Twin Stick.
 * Gestiona la instanciación y visibilidad de la interfaz de usuario.
 */
UCLASS()
class PROJECT_URO_API ATwinStickHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	ATwinStickHUD();

	// Limpieza de interfaz en BeginPlay
	virtual void BeginPlay() override;

	/** Obtiene la referencia al widget principal de la UI. */
	UFUNCTION(BlueprintCallable, Category = "TwinStick HUD")
	UTwinStickUI* GetUIWidget() const { return UIWidget; }

protected:
	/** Clase del widget de UI a instanciar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TwinStick HUD")
	TSubclassOf<UTwinStickUI> UIWidgetClass;

	/** Puntero al widget de interfaz instanciado. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TwinStick HUD", Transient)
	TObjectPtr<UTwinStickUI> UIWidget;
};
