#include "Data/PDItemSoundDataAsset.h"

#include "Sound/SoundBase.h"

USoundBase* UPDItemSoundDataAsset::GetMoveSound(EPDItemType ItemType) const
{
	switch (ItemType)
	{
	case EPDItemType::Equipment:
		return Equipment.MoveSound;
	case EPDItemType::Consumable:
		return Consumable.MoveSound;
	case EPDItemType::Misc:
		return Misc.MoveSound;
	default:
		return nullptr;
	}
}

USoundBase* UPDItemSoundDataAsset::GetConsumableUseSound() const
{
	return Consumable.UseSound;
}
