#include "Items/PDItemSoundLibrary.h"

#include "Data/PDItemSoundDataAsset.h"
#include "DeveloperSettings/PDItemSoundSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"

USoundBase* UPDItemSoundLibrary::ResolveItemMoveSound(EPDItemType ItemType)
{
	const UPDItemSoundSettings* Settings = GetDefault<UPDItemSoundSettings>();
	if (!Settings || Settings->ItemSoundDataAsset.IsNull())
	{
		return nullptr;
	}

	const UPDItemSoundDataAsset* SoundData = Settings->ItemSoundDataAsset.LoadSynchronous();
	return SoundData ? SoundData->GetMoveSound(ItemType) : nullptr;
}


USoundBase* UPDItemSoundLibrary::ResolveConsumableUseSound()
{
	const UPDItemSoundSettings* Settings = GetDefault<UPDItemSoundSettings>();
	if (!Settings || Settings->ItemSoundDataAsset.IsNull())
	{
		return nullptr;
	}

	const UPDItemSoundDataAsset* SoundData = Settings->ItemSoundDataAsset.LoadSynchronous();
	return SoundData ? SoundData->GetConsumableUseSound() : nullptr;
}

void UPDItemSoundLibrary::PlayItemMoveSound(const UObject* WorldContextObject, const FPDItemData& ItemData)
{
	if (!WorldContextObject || ItemData.ItemID.IsNone())
	{
		return;
	}

	USoundBase* Sound = ResolveItemMoveSound(ItemData.ItemType);
	if (!Sound)
	{
		return;
	}

	UGameplayStatics::PlaySound2D(WorldContextObject, Sound);
}

void UPDItemSoundLibrary::PlayConsumableUseSound(const UObject* WorldContextObject, const FPDItemData& ItemData)
{
	if (UAudioComponent* AudioComponent = SpawnConsumableUseSound(WorldContextObject, ItemData))
	{
		AudioComponent->bAutoDestroy = true;
	}
}

UAudioComponent* UPDItemSoundLibrary::SpawnConsumableUseSound(const UObject* WorldContextObject, const FPDItemData& ItemData)
{
	if (!WorldContextObject || ItemData.ItemID.IsNone() || ItemData.ItemType != EPDItemType::Consumable)
	{
		return nullptr;
	}

	USoundBase* Sound = ResolveConsumableUseSound();
	if (!Sound)
	{
		return nullptr;
	}

	return UGameplayStatics::SpawnSound2D(WorldContextObject, Sound, 1.f, 1.f, 0.f, nullptr, true, false);
}
