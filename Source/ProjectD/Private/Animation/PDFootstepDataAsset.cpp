#include "Animation/PDFootstepDataAsset.h"

const FPDFootstepEntry& UPDFootstepDataAsset::GetEntryForSurface(EPhysicalSurface Surface) const
{
	if (const FPDFootstepEntry* Found = SurfaceEntries.Find(Surface))
	{
		return *Found;
	}
	return DefaultEntry;
}