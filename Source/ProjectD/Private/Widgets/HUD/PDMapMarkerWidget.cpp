#include "Widgets/HUD/PDMapMarkerWidget.h"
#include "Ping/PDMapMarkerSubsystem.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Input/Events.h"

void UPDMapMarkerWidget::SetDisplayIndex(int32 InIndex)
{
	if (NumberText)
	{
		NumberText->SetText(FText::AsNumber(InIndex));
	}
}

FReply UPDMapMarkerWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	//우클릭 => 자기 자신 삭제 요청
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (UWorld* World = GetWorld())
		{
			if (UPDMapMarkerSubsystem* Sub = World->GetSubsystem<UPDMapMarkerSubsystem>())
			{
				Sub->RemoveMarker(MarkerId);
			}
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}