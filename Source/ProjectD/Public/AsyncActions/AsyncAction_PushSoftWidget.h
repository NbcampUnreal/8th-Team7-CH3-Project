// // Fill out your copyright notice in the Description page of Project Settings.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "Kismet/BlueprintAsyncActionBase.h"
// #include "GameplayTagContainer.h"
// #include "AsyncAction_PushSoftWidget.generated.h"
//
// class UPDActivatableBase;
//
// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPushSoftWidgetDelegate, UPDActivatableBase*, PushedWidget);
//
// /**
//  * BP에서 PrimaryLayout 스택에 위젯을 비동기로 push하기 위한 노드.
//  * 내부적으로 UPDFrontendUISubsystem::PushSoftWidgetToStackAsync 를 호출한다.
//  */
// UCLASS()
// class PROJECTD_API UAsyncAction_PushSoftWidget : public UBlueprintAsyncActionBase
// {
// 	GENERATED_BODY()
//
// public:
// 	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", HidePin = "WorldContextObject", BlueprintInternalUseOnly = "true", DisplayName = "Push Soft Widget To Widget Stack"))
// 	static UAsyncAction_PushSoftWidget* PushSoftWidget(
// 		const UObject* WorldContextObject,
// 		APlayerController* OwningPlayerController,
// 		TSoftClassPtr<UPDActivatableBase> InSoftWidgetClass,
// 		UPARAM(meta = (Categories = "Frontend.WidgetStack")) FGameplayTag InWidgetStackTag,
// 		bool bFocusOnNewlyPushedWidget = true);
//
// 	//~ Begin UBlueprintAsyncActionBase Interface
// 	virtual void Activate() override;
// 	//~ End UBlueprintAsyncActionBase Interface
//
// 	UPROPERTY(BlueprintAssignable)
// 	FOnPushSoftWidgetDelegate OnWidgetCreatedBeforePush;
//
// 	UPROPERTY(BlueprintAssignable)
// 	FOnPushSoftWidgetDelegate AfterPush;
//
// private:
// 	TWeakObjectPtr<UWorld> CachedOwningWorld;
// 	TWeakObjectPtr<APlayerController> CachedOwningPC;
// 	TSoftClassPtr<UPDActivatableBase> CachedSoftWidgetClass;
// 	FGameplayTag CachedWidgetStackTag;
// 	bool bCachedFocusOnNewlyPushedWidget = false;
// };
