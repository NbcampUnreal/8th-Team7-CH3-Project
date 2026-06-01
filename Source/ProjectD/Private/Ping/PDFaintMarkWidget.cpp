#include "Ping/PDFaintMarkWidget.h"
#include "Ping/PDPingSubsystem.h"
#include "Engine/World.h"
#include "Input/Events.h"

FReply UPDFaintMarkWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	//우클릭 시 Subsystem에 삭제 요청
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (UWorld* World = GetWorld())
		{
			if (UPDPingSubsystem* Sub = World->GetSubsystem<UPDPingSubsystem>())
			{
				Sub->RemoveFaintMark(FaintId);
			}
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}