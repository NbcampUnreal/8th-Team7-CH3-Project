// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Type/Types.h"
#include "PDFrontendUISubsystem.generated.h"

class UPDActivatableBase;
class UPDRootLayout;
class UPDWidgetStack;
enum class EWidgetInputMode : uint8;

/**
 * UI 라이프사이클 관리 서브시스템
 * HUD는 이 서브시스템이 관리하지 않음 — APDPlayerController가 직접 viewport에 추가
 */

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEffectiveUIStateChanged, EWidgetInputMode);

UCLASS()
class PROJECTD_API UPDFrontendUISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UPDFrontendUISubsystem* Get(const UObject* WorldContextObject);

	/** PC가 BeginPlay에서 RootLayout을 viewport에 추가한 뒤 등록 */
	void RegisterRootLayout(UPDRootLayout* InRootLayout);

	/** PC EndPlay에서 등록 해제 */
	void UnregisterRootLayout();

	UPDRootLayout* GetRootLayout() const { return RootLayout; }

	/** 레이어 스택 top에 화면 push. RootLayout 미등록 시 nullptr */
	UFUNCTION(BlueprintCallable, Category = "UI|Layered")
	UPDActivatableBase* PushToLayer(EUILayer Layer, TSubclassOf<UPDActivatableBase> ScreenClass);

	/** RootLayout 등록 시점에 자동으로 push할 초기 화면 요청.
	 *  RootLayout이 이미 등록되어 있으면 즉시 push, 아니면 RegisterRootLayout 시점에 flush.
	 *  Lobby GameMode의 PostLogin처럼 PC BeginPlay(=RootLayout 등록)보다 앞서 호출되는 경우를 위한 큐잉 API. */
	UFUNCTION(BlueprintCallable, Category = "UI|Layered")
	void RequestInitialPush(EUILayer Layer, TSubclassOf<UPDActivatableBase> ScreenClass);

	/** 레이어 스택 top을 pop */
	UFUNCTION(BlueprintCallable, Category = "UI|Layered")
	void PopFromLayer(EUILayer Layer);

	/** 레이어 스택 전체 정리 */
	UFUNCTION(BlueprintCallable, Category = "UI|Layered")
	void ClearLayer(EUILayer Layer);

	/** 레이어 스택 top 조회. 비어있거나 미등록이면 nullptr */
	UFUNCTION(BlueprintCallable, Category = "UI|Layered")
	UPDActivatableBase* GetTopOfLayer(EUILayer Layer) const;
	
	FOnEffectiveUIStateChanged OnEffectiveUIStateChanged;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPDRootLayout> RootLayout;

	UPROPERTY(Transient)
	TSubclassOf<UPDActivatableBase> PendingInitialPushClass;

	EUILayer PendingInitialPushLayer{};
	bool bHasPendingInitialPush = false;

	void HandleAnyStackChanged(UPDWidgetStack* ChangedStack);
	EWidgetInputMode ComputeEffectiveInputMode() const;
	void FlushPendingInitialPush();
};
