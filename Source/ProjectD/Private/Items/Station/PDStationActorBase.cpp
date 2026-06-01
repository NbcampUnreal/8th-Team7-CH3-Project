#include "Items/Station/PDStationActorBase.h"

#include "Core/PDPlayerController.h"

void APDStationActorBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindStationClose();
	Super::EndPlay(EndPlayReason);
}

void APDStationActorBase::Interact_Implementation(AActor* Interactor)
{
	APDPlayerController* PlayerController = ResolveInteractorController(Interactor);
	if (!PlayerController)
	{
		return;
	}

	if (IsStationOpen(PlayerController))
	{
		CloseStation(PlayerController);
		return;
	}

	OpenStation(PlayerController);
	BindStationClose(PlayerController);
	HandleStationOpened(PlayerController);
}

bool APDStationActorBase::IsStationOpen(APDPlayerController*) const
{
	return false;
}

void APDStationActorBase::OpenStation(APDPlayerController*)
{
}

void APDStationActorBase::CloseStation(APDPlayerController*)
{
}

bool APDStationActorBase::DidStationOpen(APDPlayerController* PlayerController) const
{
	return IsStationOpen(PlayerController);
}

void APDStationActorBase::BindStationClose(APDPlayerController* PlayerController)
{
	BoundPlayerController = PlayerController;
}

void APDStationActorBase::UnbindStationClose()
{
	BoundPlayerController.Reset();
}

void APDStationActorBase::HandleStationOpened(APDPlayerController*)
{
}

void APDStationActorBase::HandleStationClosed()
{
	UnbindStationClose();
}
