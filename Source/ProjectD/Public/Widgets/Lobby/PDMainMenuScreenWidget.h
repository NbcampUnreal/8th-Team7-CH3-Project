// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDMainMenuScreenWidget.generated.h"

class UButton;

/**
 * StartupLevel의 메인 메뉴 화면.
 * NewGame / Continue / Settings / Quit 4 버튼.
 * NewGame 클릭 시 PlayModeSelectScreenClass를 Frontend 레이어에 push.
 * Continue는 멀티 세이브 디자인 미정으로 비활성 유지.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDMainMenuScreenWidget : public UPDActivatableBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* GetDesiredFocusTarget_Implementation() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_NewGame;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Continue;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Settings;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Quit;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Menu")
	TSubclassOf<UPDActivatableBase> PlayModeSelectScreenClass;

	/** Settings 클릭 시 push할 옵션 화면(WBP_PDOptionsScreen). */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Menu")
	TSubclassOf<UPDActivatableBase> OptionsScreenClass;

private:
	UFUNCTION() void HandleNewGameClicked();
	UFUNCTION() void HandleContinueClicked();
	UFUNCTION() void HandleSettingsClicked();
	UFUNCTION() void HandleQuitClicked();
};