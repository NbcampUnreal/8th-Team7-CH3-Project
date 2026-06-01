// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PDSessionService.generated.h"

class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDOnSessionOpComplete, bool, bSuccess);

/**
 * 호스트/참가/종료 흐름을 추상화한 GameInstance 소유 객체.
 * 현재 구현체는 UPDLocalSessionService (OpenLevel?listen / ClientTravel(127.0.0.1)).
 * 향후 Steam OSS 도입 시 UPDSteamSessionService를 추가하고 UPDGameInstance에서 활성 구현체를 교체한다.
 *
 * 위젯/게임플레이 코드는 항상 이 추상 베이스 포인터만 사용하므로, 구현체 교체 시 호출부 변경이 불필요하다.
 */
UCLASS(Abstract, BlueprintType)
class PROJECTD_API UPDSessionService : public UObject
{
	GENERATED_BODY()

public:
	virtual UWorld* GetWorld() const override;

	/** 호스트로서 LobbyLevel을 listen 모드로 띄움. */
	virtual void HostSession(int32 MaxPlayers, TSoftObjectPtr<UWorld> LobbyLevel) PURE_VIRTUAL(UPDSessionService::HostSession, );

	/** 호스트의 방에 참가. 현재 구현은 127.0.0.1로 ClientTravel. */
	virtual void JoinSession(APlayerController* LocalPC) PURE_VIRTUAL(UPDSessionService::JoinSession, );

	/** 세션 종료. */
	virtual void DestroySession() PURE_VIRTUAL(UPDSessionService::DestroySession, );

	/** 방 나가기 → FallbackLevel(메인 메뉴)로 복귀.
	 *  호스트: 방을 닫고 FallbackLevel로. 클라: 서버 연결 끊고 자기만 FallbackLevel로. */
	virtual void LeaveSession(APlayerController* LocalPC, TSoftObjectPtr<UWorld> FallbackLevel) PURE_VIRTUAL(UPDSessionService::LeaveSession, );

	UPROPERTY(BlueprintAssignable, Category = "PD|Session")
	FPDOnSessionOpComplete OnHostComplete;

	UPROPERTY(BlueprintAssignable, Category = "PD|Session")
	FPDOnSessionOpComplete OnJoinComplete;

	UPROPERTY(BlueprintAssignable, Category = "PD|Session")
	FPDOnSessionOpComplete OnDestroyComplete;
};