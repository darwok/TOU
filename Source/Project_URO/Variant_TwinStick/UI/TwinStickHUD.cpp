// Copyright Epic Games, Inc. All Rights Reserved.


#include "TwinStickHUD.h"
#include "TwinStickUI.h"
#include "Blueprint/UserWidget.h"

ATwinStickHUD::ATwinStickHUD()
{
	// HUD does not need tick
	PrimaryActorTick.bCanEverTick = false;
}

void ATwinStickHUD::BeginPlay()
{
	Super::BeginPlay();

	// Ensure we have a player controller and a valid widget class assigned
	APlayerController* PC = GetOwningPlayerController();
	if (PC && UIWidgetClass)
	{
		UIWidget = CreateWidget<UTwinStickUI>(PC, UIWidgetClass);
		if (UIWidget)
		{
			UIWidget->AddToViewport(0);
		}
	}
}
