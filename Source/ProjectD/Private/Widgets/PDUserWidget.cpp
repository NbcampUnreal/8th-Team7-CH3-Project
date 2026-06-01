#include "Widgets/PDUserWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "DeveloperSettings/PDUISoundSettings.h"
#include "Kismet/GameplayStatics.h"

void UPDUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindButtonClickSounds();
}

void UPDUserWidget::NativeDestruct()
{
	UnbindButtonClickSounds();
	Super::NativeDestruct();
}

void UPDUserWidget::HandleButtonClickedSound()
{
	if (USoundBase* Sound = ResolveButtonClickSound())
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

void UPDUserWidget::BindButtonClickSounds()
{
	UnbindButtonClickSounds();

	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		UButton* Button = Cast<UButton>(Widget);
		if (!Button)
		{
			return;
		}

		Button->OnClicked.AddUniqueDynamic(this, &UPDUserWidget::HandleButtonClickedSound);
		BoundSoundButtons.Add(Button);
	});
}

void UPDUserWidget::UnbindButtonClickSounds()
{
	for (UButton* Button : BoundSoundButtons)
	{
		if (Button)
		{
			Button->OnClicked.RemoveDynamic(this, &UPDUserWidget::HandleButtonClickedSound);
		}
	}

	BoundSoundButtons.Reset();
}

USoundBase* UPDUserWidget::ResolveButtonClickSound() const
{
	const UPDUISoundSettings* Settings = GetDefault<UPDUISoundSettings>();
	if (!Settings)
	{
		return nullptr;
	}

	return Settings->ButtonClickSound.LoadSynchronous();
}
