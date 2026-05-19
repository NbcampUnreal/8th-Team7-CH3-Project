// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDLobbyScreenWidget.generated.h"

class UButton;


UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDLobbyScreenWidget : public UPDActivatableBase
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

	/** Lobby "새 게임" 버튼이 진입할 게임 레벨. WBP에서 지정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Levels")
	TSoftObjectPtr<UWorld> MainLevel;

private:
	UFUNCTION() void HandleNewGameClicked();
	UFUNCTION() void HandleContinueClicked();
	UFUNCTION() void HandleSettingsClicked();
	UFUNCTION() void HandleQuitClicked();
};