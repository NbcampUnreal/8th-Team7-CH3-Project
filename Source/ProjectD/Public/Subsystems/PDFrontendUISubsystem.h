// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PDFrontendUISubsystem.generated.h"

class UPDActivatableBase;

/**
 * 단일 메뉴 라이프사이클을 관리하는 서브시스템
 * 한 번에 활성 메뉴는 하나(CurrentScreen)만 배치. 새 화면을 열면 기존 화면을 자동으로 닫음.
 * HUD는 이 서브시스템이 관리하지 않음 (APDPlayerController가 BeginPlay에서 직접 viewport에 추가)
 * 모달은 메뉴 위젯 내부의 sub-widget으로 visibility 토글
 */
UCLASS()
class PROJECTD_API UPDFrontendUISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UPDFrontendUISubsystem* Get(const UObject* WorldContextObject);

	/** 새 화면 열기. 기존 CurrentScreen이 있으면 자동으로 닫고 새 화면으로 교체. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	UPDActivatableBase* OpenScreen(TSubclassOf<UPDActivatableBase> ScreenClass);

	/** 현재 화면 닫기. 없으면 return */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void CloseScreen();

	/** 현재 활성 화면. 없으면 nullptr. */
	UFUNCTION(BlueprintPure, Category = "UI")
	UPDActivatableBase* GetCurrentScreen() const { return CurrentScreen; }

private:
	/** AddToViewport ZOrder. HUD(0) 위, 시스템 모달(필요해질 경우 더 높은 값)의 아래. */
	static constexpr int32 ScreenZOrder = 10;

	UPROPERTY(Transient)
	TObjectPtr<UPDActivatableBase> CurrentScreen;
};
