#include "Items/PDMarketActor.h"

#include "Components/BoxComponent.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Items/PDMarketComponent.h"

APDMarketActor::APDMarketActor()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionCollision"));
	SetRootComponent(InteractionCollision);
	InteractionCollision->SetBoxExtent(FVector(80.f, 80.f, 80.f));
	ConfigureInteractionCollision();

	MarketComponent = CreateDefaultSubobject<UPDMarketComponent>(TEXT("MarketComponent"));
}

void APDMarketActor::BeginPlay()
{
	Super::BeginPlay();

	ConfigureInteractionCollision();
}

void APDMarketActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ConfigureInteractionCollision();
}

void APDMarketActor::ConfigureInteractionCollision() const
{
	if (!InteractionCollision)
	{
		return;
	}

	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollision->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionCollision->SetGenerateOverlapEvents(true);
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
