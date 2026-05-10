#include "Items/PDMarketActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Items/PDMarketComponent.h"

APDMarketActor::APDMarketActor()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionCollision"));
	SetRootComponent(InteractionCollision);
	InteractionCollision->SetBoxExtent(FVector(80.f, 80.f, 80.f));
	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollision->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionCollision->SetGenerateOverlapEvents(false);

	MarketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarketMesh"));
	MarketMesh->SetupAttachment(InteractionCollision);
	MarketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MarketComponent = CreateDefaultSubobject<UPDMarketComponent>(TEXT("MarketComponent"));
}

void APDMarketActor::Interact_Implementation(AActor* Interactor)
{
	APawn* InteractingPawn = Cast<APawn>(Interactor);
	if (!InteractingPawn)
	{
		return;
	}

	APDPlayerController* PlayerController = Cast<APDPlayerController>(InteractingPawn->GetController());
	if (!PlayerController)
	{
		return;
	}

	if (PlayerController->IsMarketInterfaceOpen())
	{
		PlayerController->CloseMarketInterface();
		return;
	}

	PlayerController->OpenMarketInterface(MarketComponent);
}
