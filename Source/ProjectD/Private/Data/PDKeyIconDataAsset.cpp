#include "Data/PDKeyIconDataAsset.h"

#include "Engine/Texture2D.h"

UTexture2D* UPDKeyIconDataAsset::ResolveIcon(const FKey& Key) const
{
	if (const TSoftObjectPtr<UTexture2D>* Found = KeyIconMap.Find(Key))
	{
		if (UTexture2D* Loaded = Found->LoadSynchronous())
		{
			return Loaded;
		}
	}
	return FallbackIcon.LoadSynchronous();
}