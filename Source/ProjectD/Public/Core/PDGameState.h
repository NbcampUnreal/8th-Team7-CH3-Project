// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Type/Types.h"
#include "PDGameState.generated.h"

// 존 카운트다운 상태가 바뀔 때(시작/단축/취소/인원변경) 위젯 갱신용. 서버/클라 모두 발화.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDOnZoneCountdownChanged);

// AGameModeBase 와 짝이 맞는 AGameStateBase 상속. (AGameState 풀버전은 AGameMode 전용 — MatchState 미진행으로 입력 게이팅 문제 발생)
UCLASS()
class PROJECTD_API APDGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	void SetRaidState(ERaidState NewState);
	FORCEINLINE ERaidState GetRaidState() const{return CurrentRaidState;}

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ─── 존 트래블 카운트다운 (서버 권한, 모든 클라에 복제) ───────────────────
	// 서버 전용: GameMode 가 존 점유 평가 결과를 반영.
	void SetZoneCountdown(EPDZoneTravelType InType, int32 InPlayersInZone, int32 InTotal, float InEndServerTime, bool bInFinal);
	void ClearZoneCountdown();

	UFUNCTION(BlueprintPure, Category="PD|Zone")
	EPDZoneTravelType GetZoneTravelType() const { return ZoneTravelType; }

	UFUNCTION(BlueprintPure, Category="PD|Zone")
	bool IsZoneCountdownActive() const { return ZoneTravelType != EPDZoneTravelType::None; }

	UFUNCTION(BlueprintPure, Category="PD|Zone")
	int32 GetZonePlayersInZone() const { return ZonePlayersInZone; }

	UFUNCTION(BlueprintPure, Category="PD|Zone")
	int32 GetZoneTotalParticipants() const { return ZoneTotalParticipants; }

	UFUNCTION(BlueprintPure, Category="PD|Zone")
	bool IsZoneFinalCountdown() const { return bZoneFinalCountdown; }

	// 서버 시간 기준으로 계산한 남은 초(클라/서버 공통, 0 이상). 비활성이면 0.
	UFUNCTION(BlueprintPure, Category="PD|Zone")
	float GetZoneCountdownRemaining() const;

	// 위젯이 표시/숨김 및 갱신을 위해 바인딩.
	UPROPERTY(BlueprintAssignable, Category="PD|Zone")
	FPDOnZoneCountdownChanged OnZoneCountdownChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Raid")
	ERaidState CurrentRaidState=ERaidState::Idle;

	UFUNCTION(BlueprintImplementableEvent, Category="PD|Raid")
	void OnRaidStateChanged(ERaidState NewState);

	UPROPERTY(ReplicatedUsing=OnRep_ZoneCountdown, VisibleAnywhere, BlueprintReadOnly, Category="PD|Zone")
	EPDZoneTravelType ZoneTravelType = EPDZoneTravelType::None;

	UPROPERTY(ReplicatedUsing=OnRep_ZoneCountdown, VisibleAnywhere, BlueprintReadOnly, Category="PD|Zone")
	int32 ZonePlayersInZone = 0;

	UPROPERTY(ReplicatedUsing=OnRep_ZoneCountdown, VisibleAnywhere, BlueprintReadOnly, Category="PD|Zone")
	int32 ZoneTotalParticipants = 0;

	// 카운트다운 만료 서버 시간(GetServerWorldTimeSeconds 기준). 비활성이면 -1.
	UPROPERTY(ReplicatedUsing=OnRep_ZoneCountdown, VisibleAnywhere, BlueprintReadOnly, Category="PD|Zone")
	float ZoneCountdownEndServerTime = -1.f;

	UPROPERTY(ReplicatedUsing=OnRep_ZoneCountdown, VisibleAnywhere, BlueprintReadOnly, Category="PD|Zone")
	bool bZoneFinalCountdown = false;

	UFUNCTION()
	void OnRep_ZoneCountdown();
};