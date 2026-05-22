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

FReply UPDScreenTemplateBase::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (HandleEscape())
		{
			return FReply::Handled();
		}
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

bool UPDScreenTemplateBase::HandleEscape_Implementation()
{
	if (ActiveModalWidget)
	{
		DismissModal();
		return true;
	}

	UPDFrontendUISubsystem* Subsystem = UPDFrontendUISubsystem::Get(this);
	if (!Subsystem)
	{
		return false;
	}

	const EUILayer LayerSearchOrder[] = { EUILayer::Modal, EUILayer::GameMenu, EUILayer::Frontend };
	for (EUILayer Layer : LayerSearchOrder)
	{
		if (Subsystem->GetTopOfLayer(Layer) == this)
		{
			Subsystem->PopFromLayer(Layer);
			return true;
		}
	}
	return false;
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