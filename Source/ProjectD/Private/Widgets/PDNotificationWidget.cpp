#include "Widgets/PDNotificationWidget.h"

#include "Components/TextBlock.h"
#include "TimerManager.h"

void UPDNotificationWidget::ShowNotification(const FText& Message, float Duration)
{
	if (Text_Message)
	{
		Text_Message->SetText(Message);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(1.f);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
		World->GetTimerManager().SetTimer(HideTimerHandle, this, &UPDNotificationWidget::HideNotification, FMath::Max(0.1f, Duration), false);
	}
}

void UPDNotificationWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
	}

	Super::NativeDestruct();
}

void UPDNotificationWidget::HideNotification()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
