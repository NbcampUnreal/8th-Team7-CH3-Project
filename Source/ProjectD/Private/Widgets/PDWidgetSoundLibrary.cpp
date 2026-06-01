#include "Widgets/PDWidgetSoundLibrary.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void UPDWidgetSoundLibrary::PlayUISound2D(const UObject* WorldContextObject, USoundBase* Sound)
{
	if (!WorldContextObject || !Sound)
	{
		return;
	}

	UGameplayStatics::PlaySound2D(WorldContextObject, Sound);
}
