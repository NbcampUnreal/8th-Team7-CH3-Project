#include "Items/PDEquipmentModificationActor.h"

#include "Components/BoxComponent.h"
#include "Component/PDInteractionOutlineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"

APDEquipmentModificationActor::APDEquipmentModificationActor()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionCollision"));
	SetRootComponent(InteractionCollision);
	InteractionCollision->SetBoxExtent(FVector(80.f, 80.f, 80.f));
	ConfigureInteractionCollision();

	WorkbenchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorkbenchMesh"));
	WorkbenchMesh->SetupAttachment(InteractionCollision);
	WorkbenchMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	OutlineComponent = CreateDefaultSubobject<UPDInteractionOutlineComponent>(TEXT("OutlineComponent"));
	OutlineComponent->SetupTrigger(InteractionCollision);
}

void APDEquipmentModificationActor::BeginPlay()
{
	Super::BeginPlay();

	ConfigureInteractionCollision();
	if (OutlineComponent)
	{
		OutlineComponent->SetupTrigger(InteractionCollision);
	}
}

void APDEquipmentModificationActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindEquipmentModificationClose();
	Super::EndPlay(EndPlayReason);
}

void APDEquipmentModificationActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ConfigureInteractionCollision();
}

void APDEquipmentModificationActor::ConfigureInteractionCollision() const
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

void APDEquipmentModificationActor::Interact_Implementation(AActor* Interactor)
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

	if (PlayerController->IsEquipmentModificationInterfaceOpen())
	{
		PlayerController->CloseEquipmentModificationInterface();
		return;
	}

	PlayerController->OpenEquipmentModificationInterface();

	if (PlayerController->IsEquipmentModificationInterfaceOpen())
	{
		BindEquipmentModificationClose(PlayerController);
		OnEquipmentModificationOpened.Broadcast(this);
	}
}

void APDEquipmentModificationActor::BindEquipmentModificationClose(APDPlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	UnbindEquipmentModificationClose();
	BoundPlayerController = PlayerController;
	PlayerController->OnEquipmentModificationInterfaceClosed.AddUObject(this, &APDEquipmentModificationActor::HandleEquipmentModificationInterfaceClosed);
}

void APDEquipmentModificationActor::UnbindEquipmentModificationClose()
{
	if (BoundPlayerController.IsValid())
	{
		BoundPlayerController->OnEquipmentModificationInterfaceClosed.RemoveAll(this);
	}

	BoundPlayerController.Reset();
}

void APDEquipmentModificationActor::HandleEquipmentModificationInterfaceClosed()
{
	UnbindEquipmentModificationClose();
	OnEquipmentModificationClosed.Broadcast(this);
}
