// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDLobbyPlayerEntryWidget.generated.h"

class UTextBlock;
class UWidget;
class APlayerState;

/**
 * Lobby PlayerList의 한 슬롯.
 * 채워진 슬롯: SetPlayerState(PS, bIsHost) — PlayerName + (호스트면) HOST 뱃지.
 * 빈 슬롯: SetEmpty() — "Empty" 표시, 뱃지 숨김.
 * 클라이언트에서 PlayerName이 늦게 도착하는 케이스를 짧은 폴링으로 보정.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDLobbyPlayerEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPlayerState(APlayerState* InPlayerState, bool bInIsHost);
	void SetEmpty();

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlock_PlayerName;

	/** "HOST" 라벨. 호스트 슬롯에서만 Visible. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> HostBadge;

private:
	UFUNCTION()
	void RefreshName();

	void SetHostBadgeVisible(bool bVisible);

	UPROPERTY()
	TWeakObjectPtr<APlayerState> CachedPlayerState;

	FTimerHandle PollTimerHandle;
	int32 PollAttempts = 0;
};