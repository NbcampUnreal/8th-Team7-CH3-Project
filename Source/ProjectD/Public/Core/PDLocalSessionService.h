// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/PDSessionService.h"
#include "PDLocalSessionService.generated.h"

/**
 * Steam OSS를 사용하지 않는 현 단계용 SessionService 구현체.
 * HostSession  → OpenLevel(LobbyMap?listen)
 * JoinSession  → ClientTravel("127.0.0.1") (PIE/LAN 가정)
 * DestroySession → no-op (별도 세션 객체 없음)
 */
UCLASS()
class PROJECTD_API UPDLocalSessionService : public UPDSessionService
{
	GENERATED_BODY()

public:
	virtual void HostSession(int32 MaxPlayers, TSoftObjectPtr<UWorld> LobbyLevel) override;
	virtual void JoinSession(APlayerController* LocalPC) override;
	virtual void DestroySession() override;
	virtual void LeaveSession(APlayerController* LocalPC, TSoftObjectPtr<UWorld> FallbackLevel) override;
};