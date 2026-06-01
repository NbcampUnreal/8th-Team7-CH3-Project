// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/PDStartupGameMode.h"

#include "Core/PDStartupPlayerController.h"

APDStartupGameMode::APDStartupGameMode()
{
	PlayerControllerClass = APDStartupPlayerController::StaticClass();
}