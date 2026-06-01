#include "Items/World/PDItemBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Characters/PDPlayerCharacter.h"
#include "Core/PDPlayerComponentResolver.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Engine/DataTable.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayTag/PDGameplayTags.h"

namespace
{
	const FPDItemData* FindPDItemBaseItemDataByID(const UDataTable* DataTable, const FName& ItemID)
	{
		if (!DataTable || ItemID.IsNone())
		{
			return nullptr;
		}

		TArray<FPDItemData*> Rows;
		DataTable->GetAllRows<FPDItemData>(TEXT("FindPDItemBaseItemDataByID"), Rows);

		for (const FPDItemData* Row : Rows)
		{
			if (Row && Row->ItemID == ItemID)
			{
				return Row;
			}
		}

		return nullptr;
	}
}

APDItemBase::APDItemBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

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
	if (!HasAuthority()) return;

	if (!Interactor || Quantity <= 0)
	{
		OnPickupFailed();
		return;
	}

	UPDInventoryComponent* Inventory = FPDPlayerComponentResolver::ResolveInventory(Interactor);
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

	if (UAbilitySystemComponent* ASCComp = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Interactor))
	{
		FGameplayCueParameters Params;
		Params.Location = GetActorLocation();
		Params.SourceObject = this;
		Params.RawMagnitude = static_cast<float>(AddedQuantity);

		ASCComp->ExecuteGameplayCue(PDGameplayTags::GameplayCue_Item_Pickup, Params);
	}

	OnPickupSucceeded(AddedQuantity, Quantity);

	if (Quantity <= 0)
	{
		Destroy();
	}
}

void APDItemBase::LoadItemData()
{
	if (!ItemDataTable || ItemRowName.IsNone()) return;

	const FPDItemData* Row = FindPDItemBaseItemDataByID(ItemDataTable, ItemRowName);
	if (!Row) return;

	CachedItemData = *Row;
	if (CachedItemData.ItemID.IsNone())
	{
		CachedItemData.ItemID = ItemRowName;
	}
	OnItemDataLoaded();
}
