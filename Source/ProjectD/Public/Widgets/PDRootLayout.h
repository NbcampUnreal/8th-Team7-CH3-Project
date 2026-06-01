// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Type/Types.h"
#include "PDRootLayout.generated.h"

class UPDWidgetStack;

/**
 * 전역 UI 컨테이너. PC가 BeginPlay에서 1회 생성
 * 게임 시작부터 종료까지 viewport에 상주하며 각 레이어의 스택을 보유
 */
UCLASS(Abstract)
class PROJECTD_API UPDRootLayout : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 레이어 enum으로 해당 스택을 조회. BindWidget 실패 시 nullptr. */
	UPDWidgetStack* GetLayerStack(EUILayer Layer) const;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDWidgetStack> Layer_Frontend;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDWidgetStack> Layer_GameMenu;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDWidgetStack> Layer_Modal;
};