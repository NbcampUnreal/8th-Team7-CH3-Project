// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDPlayModeSelectScreenWidget.generated.h"

class UButton;

/**
 * NewGame 다음 단계: 호스트로 방을 만들지, 호스트의 방에 참가할지 선택.
 * 환경별 분기:
 *   - 이미 연결됨(PIE Listen Server / Client) → 세션 작업 없이 LobbyScreenClass(방 화면)로 전환만
 *   - Standalone(빌드/Steam) → SessionService로 실제 호스팅/참가
 * Back: Frontend 레이어 pop → 메인 메뉴 복귀
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDPlayModeSelectScreenWidget : public UPDActivatableBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* GetDesiredFocusTarget_Implementation() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Host;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Join;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Back;

	/** Host/Join 후 push할 방 화면(WBP_PDLobbyScreenWidget). */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Menu")
	TSubclassOf<UPDActivatableBase> LobbyScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Menu", meta = (ClampMin = "1"))
	int32 MaxLobbyPlayers = 4;

private:
	UFUNCTION() void HandleHostClicked();
	UFUNCTION() void HandleJoinClicked();
	UFUNCTION() void HandleBackClicked();

	/** 이미 서버에 연결된 상태(Listen Server/Client)면 true → 세션 작업 불필요. */
	bool IsAlreadyConnected() const;
	void PushLobbyScreen();
};