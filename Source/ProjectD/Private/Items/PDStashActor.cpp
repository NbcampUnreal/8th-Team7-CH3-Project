#include "Items/PDStashActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"

APDStashActor::APDStashActor()
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

	StashMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StashMesh"));
	StashMesh->SetupAttachment(InteractionCollision);
	StashMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APDStashActor::Interact_Implementation(AActor* Interactor)
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

	if (PlayerController->IsStashInterfaceOpen())
	{
		PlayerController->CloseStashInterface();
		return;
	}

	PlayerController->OpenStashInterface();
}
