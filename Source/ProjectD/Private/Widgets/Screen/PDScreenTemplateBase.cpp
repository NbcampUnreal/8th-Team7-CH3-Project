#include "Widgets/Screen/PDScreenTemplateBase.h"

#include "Components/NamedSlot.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Type/Types.h"

void UPDScreenTemplateBase::NativeOnActivated()
{
	Super::NativeOnActivated();
}

void UPDScreenTemplateBase::NativeOnDeactivated()
{
	DismissModal();
	Super::NativeOnDeactivated();
}

bool UPDScreenTemplateBase::HandleEscape_Implementation()
{
	// 화면 chrome 내부 모달이 떠있으면 그것부터 닫는다. layer pop은 모달이 없을 때만.
	if (ActiveModalWidget)
	{
		DismissModal();
		return true;
	}
	return Super::HandleEscape_Implementation();
}

void UPDScreenTemplateBase::SetDescription(const FText& NewDescription)
{
	if (!NamedSlot_Description)
	{
		return;
	}

	if (UWidget* Child = NamedSlot_Description->GetChildAt(0))
	{
		if (UTextBlock* TextChild = Cast<UTextBlock>(Child))
		{
			TextChild->SetText(NewDescription);
		}
	}

	NamedSlot_Description->SetVisibility(
		NewDescription.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UPDScreenTemplateBase::ShowModal(UUserWidget* ModalWidget)
{
	if (!ModalWidget || !NamedSlot_Modal)
	{
		return;
	}

	if (ActiveModalWidget)
	{
		DismissModal();
	}

	NamedSlot_Modal->ClearChildren();
	NamedSlot_Modal->AddChild(ModalWidget);
	NamedSlot_Modal->SetVisibility(ESlateVisibility::Visible);
	ActiveModalWidget = ModalWidget;
}

void UPDScreenTemplateBase::DismissModal()
{
	if (NamedSlot_Modal)
	{
		NamedSlot_Modal->ClearChildren();
		NamedSlot_Modal->SetVisibility(ESlateVisibility::Collapsed);
	}
	ActiveModalWidget = nullptr;
}

UPDScreenTemplateBase* UPDScreenTemplateBase::FindForWidget(UWidget* Source)
{
	if (!Source)
	{
		return nullptr;
	}
	return Source->GetTypedOuter<UPDScreenTemplateBase>();
}