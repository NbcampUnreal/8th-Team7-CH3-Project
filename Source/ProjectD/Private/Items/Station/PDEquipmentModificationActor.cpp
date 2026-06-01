#include "Items/Station/PDEquipmentModificationActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/PDPlayerController.h"

APDEquipmentModificationActor::APDEquipmentModificationActor()
{
	PrimaryActorTick.bCanEverTick = false;

	WorkbenchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorkbenchMesh"));
	WorkbenchMesh->SetupAttachment(InteractionCollision);
	WorkbenchMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool APDEquipmentModificationActor::IsStationOpen(APDPlayerController* PlayerController) const
{
	return PlayerController && PlayerController->IsEquipmentModificationInterfaceOpen();
}

void APDEquipmentModificationActor::OpenStation(APDPlayerController* PlayerController)
{
	if (PlayerController)
	{
		PlayerController->OpenEquipmentModificationInterface();
	}
}

void APDEquipmentModificationActor::CloseStation(APDPlayerController* PlayerController)
{
	if (PlayerController)
	{
		PlayerController->CloseEquipmentModificationInterface();
	}
}

void APDEquipmentModificationActor::BindStationClose(APDPlayerController* PlayerController)
{
	UnbindStationClose();
	BoundPlayerController = PlayerController;
	if (PlayerController)
	{
		PlayerController->OnEquipmentModificationInterfaceClosed.AddUObject(this, &APDEquipmentModificationActor::HandleEquipmentModificationInterfaceClosed);
	}
}

void APDEquipmentModificationActor::UnbindStationClose()
{
	if (BoundPlayerController.IsValid())
	{
		BoundPlayerController->OnEquipmentModificationInterfaceClosed.RemoveAll(this);
	}

	Super::UnbindStationClose();
}

void APDEquipmentModificationActor::HandleStationOpened(APDPlayerController*)
{
	OnEquipmentModificationOpened.Broadcast(this);
}

void APDEquipmentModificationActor::HandleEquipmentModificationInterfaceClosed()
{
	HandleStationClosed();
	OnEquipmentModificationClosed.Broadcast(this);
}
