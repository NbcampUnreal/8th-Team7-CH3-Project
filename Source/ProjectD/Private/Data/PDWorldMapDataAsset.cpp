#include "Data/PDWorldMapDataAsset.h"
#include "Engine/World.h"
#include "UObject/Package.h"

FPDWorldMapEntry UPDWorldMapDataAsset::GetEntryForWorld(const UWorld* InWorld) const
{
    if (!InWorld) return DefaultEntry;

    
    FString CurrentPath = InWorld->GetPackage()->GetName();
    UWorld::RemovePIEPrefix(CurrentPath);

    for (const TPair<TSoftObjectPtr<UWorld>, FPDWorldMapEntry>& Pair : LevelMaps)
    {
        const FString KeyPath = Pair.Key.ToSoftObjectPath().GetLongPackageName();
        if (CurrentPath == KeyPath)
        {
            return Pair.Value;
        }
    }

    return DefaultEntry;
}