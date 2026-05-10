#include "Items/PDItemBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Items/PDInventoryComponent.h"

APDItemBase::APDItemBase()
{
	PrimaryActorTick.bCanEverTick = false;

	PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
	SetRootComponent(PickupCollision);
	PickupCollision->InitSphereRadius(80.f);
	PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupCollision->SetCollisionObjectType(ECC_WorldDynamic);
	PickupCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	PickupCollision->SetGenerateOverlapEvents(false);

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(PickupCollision);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APDItemBase::BeginPlay()
{
	Super::BeginPlay();
	LoadItemData();
}

void APDItemBase::Interact_Implementation(AActor* Interactor)
{
	if (!Interactor || Quantity <= 0)
	{
		OnPickupFailed();
		return;
	}

	UPDInventoryComponent* Inventory = Interactor->FindComponentByClass<UPDInventoryComponent>();

	if (!Inventory)
	{
		OnPickupFailed();
		return;
	}

	const FPDItemData& ItemDataToAdd = CachedItemData;

	if (ItemDataToAdd.ItemID.IsNone())
	{
		OnPickupFailed();
		return;
	}

	const int32 AddedQuantity = Inventory->AddItemPartial(ItemDataToAdd, Quantity);

	if (AddedQuantity <= 0)
	{
		OnPickupFailed();
		return;
	}

	Quantity -= AddedQuantity;
	OnPickupSucceeded(AddedQuantity, Quantity);

	if (Quantity <= 0)
	{
		Destroy();
	}
}

void APDItemBase::LoadItemData()
{
	if (!ItemDataTable || ItemRowName.IsNone()) return;

	const FPDItemData* Row = ItemDataTable->FindRow<FPDItemData>(ItemRowName, TEXT("PDItemBase"));
	if (!Row) return;

	CachedItemData = *Row;
	if (CachedItemData.ItemID.IsNone())
	{
		CachedItemData.ItemID = ItemRowName;
	}
	OnItemDataLoaded();
}
