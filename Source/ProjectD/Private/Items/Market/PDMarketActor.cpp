#include "Items/Market/PDMarketActor.h"

#include "Core/PDPlayerController.h"
#include "Items/Market/PDMarketComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

APDMarketActor::APDMarketActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MarketComponent = CreateDefaultSubobject<UPDMarketComponent>(TEXT("MarketComponent"));
}

bool APDMarketActor::IsStationOpen(APDPlayerController* PlayerController) const
{
	return PlayerController &&
		PlayerController->IsMarketInterfaceOpen() &&
		PlayerController->GetActiveMarketComponent() == MarketComponent;
}

void APDMarketActor::OpenStation(APDPlayerController* PlayerController)
{
	if (PlayerController)
	{
		PlayerController->OpenMarketInterface(MarketComponent);
	}
}

void APDMarketActor::CloseStation(APDPlayerController* PlayerController)
{
	if (PlayerController)
	{
		PlayerController->CloseMarketInterface();
	}
}

void APDMarketActor::BindStationClose(APDPlayerController* PlayerController)
{
	UnbindStationClose();
	BoundPlayerController = PlayerController;
	if (PlayerController)
	{
		PlayerController->OnMarketInterfaceClosed.AddUObject(this, &APDMarketActor::HandleMarketInterfaceClosed);
	}
}

void APDMarketActor::UnbindStationClose()
{
	if (BoundPlayerController.IsValid())
	{
		BoundPlayerController->OnMarketInterfaceClosed.RemoveAll(this);
	}

	Super::UnbindStationClose();
}

void APDMarketActor::HandleStationOpened(APDPlayerController*)
{
	if (MarketOpenSound)
	{
		UGameplayStatics::PlaySound2D(this, MarketOpenSound);
	}
	OnMarketOpened.Broadcast(this);
}

void APDMarketActor::HandleMarketInterfaceClosed(UPDMarketComponent* ClosedMarketComponent)
{
	if (ClosedMarketComponent != MarketComponent)
	{
		return;
	}

	if (MarketCloseSound)
	{
		UGameplayStatics::PlaySound2D(this, MarketCloseSound);
	}
	HandleStationClosed();
	OnMarketClosed.Broadcast(this);
}
