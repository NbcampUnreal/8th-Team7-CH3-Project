// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDZoneCountdownWidget.generated.h"

class UTextBlock;
class APDGameState;

/**
 * 존(RaidEntry/Extraction) 트래블 카운트다운 HUD 위젯.
 * APDGameState 의 복제된 존 상태를 폴링해 "존 인원 / 총원" + 남은 초를 표시.
 * 카운트다운 비활성(ZoneType=None)이면 자동 숨김. 비주얼/애니메이션은 BP에서.
 */
UCLASS(Abstract)
class PROJECTD_API UPDZoneCountdownWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 카운트다운 표시/숨김 전환 시 1회 호출. BP에서 등장/퇴장 애니메이션 등에 사용.
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Zone")
	void OnCountdownVisibilityChanged(bool bVisible);

	// 표시 중 매 프레임 값 갱신. BindWidget 텍스트가 없으면 BP에서 직접 표시 가능.
	// bFinalCountdown = 전원 진입으로 3초 단축된 상태(긴급 색상 등에 사용).
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Zone")
	void OnCountdownUpdated(int32 PlayersInZone, int32 TotalParticipants, int32 RemainingSeconds, bool bFinalCountdown);

	// 있으면 C++가 직접 채움("X / Y"). 없으면 BP가 OnCountdownUpdated 로 처리.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> Text_ZoneCount = nullptr;

	// 있으면 C++가 직접 채움(남은 초). 없으면 BP가 OnCountdownUpdated 로 처리.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> Text_Countdown = nullptr;

private:
	APDGameState* GetPDGameState() const;
	void Refresh();

	bool bWasActive = false;

	// NativeTick 은 위젯이 Collapsed 면 안 불리므로(데드락), 타이머로 폴링해 자기 자신을 Collapse 해도 계속 갱신.
	FTimerHandle RefreshTimerHandle;
};