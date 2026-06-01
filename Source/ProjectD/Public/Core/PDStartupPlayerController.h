// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/PDLobbyPlayerController.h"
#include "PDStartupPlayerController.generated.h"

/**
 * StartupLevel(메인 메뉴 단계) 전용 PlayerController.
 * 동작은 APDLobbyPlayerController와 동일 — BeginPlay 시 LobbyScreenClass push.
 * BP 디폴트에서 LobbyScreenClass = WBP_PDMainMenuScreen 으로 설정해 사용한다.
 */
UCLASS()
class PROJECTD_API APDStartupPlayerController : public APDLobbyPlayerController
{
	GENERATED_BODY()
};