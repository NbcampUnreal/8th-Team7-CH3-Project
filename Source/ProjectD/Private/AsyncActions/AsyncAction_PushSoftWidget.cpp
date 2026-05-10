// // Fill out your copyright notice in the Description page of Project Settings.
//
//
// #include "AsyncActions/AsyncAction_PushSoftWidget.h"
//
// #include "Subsystems/PDFrontendUISubsystem.h"
// #include "Widgets/PDActivatableBase.h"
//
// UAsyncAction_PushSoftWidget* UAsyncAction_PushSoftWidget::PushSoftWidget(
// 	const UObject* WorldContextObject,
// 	APlayerController* OwningPlayerController,
// 	TSoftClassPtr<UPDActivatableBase> InSoftWidgetClass,
// 	FGameplayTag InWidgetStackTag,
// 	bool bFocusOnNewlyPushedWidget)
// {
// 	checkf(!InSoftWidgetClass.IsNull(), TEXT("PushSoftWidget: SoftWidgetClass가 null입니다."));
//
// 	if (GEngine)
// 	{
// 		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
// 		{
// 			UAsyncAction_PushSoftWidget* Node = NewObject<UAsyncAction_PushSoftWidget>();
// 			Node->CachedOwningWorld = World;
// 			Node->CachedOwningPC = OwningPlayerController;
// 			Node->CachedSoftWidgetClass = InSoftWidgetClass;
// 			Node->CachedWidgetStackTag = InWidgetStackTag;
// 			Node->bCachedFocusOnNewlyPushedWidget = bFocusOnNewlyPushedWidget;
//
// 			Node->RegisterWithGameInstance(World);
// 			return Node;
// 		}
// 	}
//
// 	return nullptr;
// }
//
// void UAsyncAction_PushSoftWidget::Activate()
// {
// 	UPDFrontendUISubsystem* Subsystem = UPDFrontendUISubsystem::Get(CachedOwningWorld.Get());
//
// 	Subsystem->PushSoftWidgetToStackAsync(
// 		CachedWidgetStackTag,
// 		CachedSoftWidgetClass,
// 		[this](EAsyncPushWidgetState InPushState, UPDActivatableBase* PushedWidget)
// 		{
// 			switch (InPushState)
// 			{
// 			case EAsyncPushWidgetState::OnCreatedBeforePush:
// 				PushedWidget->SetOwningPlayer(CachedOwningPC.Get());
// 				OnWidgetCreatedBeforePush.Broadcast(PushedWidget);
// 				break;
//
// 			case EAsyncPushWidgetState::AfterPush:
// 				AfterPush.Broadcast(PushedWidget);
// 				if (bCachedFocusOnNewlyPushedWidget)
// 				{
// 					if (UWidget* WidgetToFocus = PushedWidget->GetDesiredFocusTarget())
// 					{
// 						WidgetToFocus->SetFocus();
// 					}
// 				}
// 				SetReadyToDestroy();
// 				break;
// 			default:
// 				break;
// 			}
// 		}
// 	);
// }
