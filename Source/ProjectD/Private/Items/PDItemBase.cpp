#include "Items/PDItemBase.h"

APDItemBase::APDItemBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APDItemBase::BeginPlay()
{
	Super::BeginPlay();
	LoadItemData();
}

void APDItemBase::LoadItemData()
{
	if (!ItemDataTable || ItemRowName.IsNone()) return;

	const FPDItemData* Row = ItemDataTable->FindRow<FPDItemData>(ItemRowName, TEXT("PDItemBase"));
	if (!Row) return;

	CachedItemData = *Row;
	OnItemDataLoaded();
}
