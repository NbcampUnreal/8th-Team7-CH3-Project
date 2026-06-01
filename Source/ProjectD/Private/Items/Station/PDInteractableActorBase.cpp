#include "Items/Station/PDInteractableActorBase.h"

#include "Component/PDInteractionOutlineComponent.h"
#include "Components/BoxComponent.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"

APDInteractableActorBase::APDInteractableActorBase()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionCollision"));
	SetRootComponent(InteractionCollision);
	InteractionCollision->SetBoxExtent(FVector(80.f, 80.f, 80.f));
	ConfigureInteractionCollision();

	OutlineComponent = CreateDefaultSubobject<UPDInteractionOutlineComponent>(TEXT("OutlineComponent"));
	RefreshOutlineBinding();
}

void APDInteractableActorBase::BeginPlay()
{
	Super::BeginPlay();

	ConfigureInteractionCollision();
	RefreshOutlineBinding();
}

void APDInteractableActorBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ConfigureInteractionCollision();
	RefreshOutlineBinding();
}

void APDInteractableActorBase::ConfigureInteractionCollision() const
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

void APDInteractableActorBase::RefreshOutlineBinding() const
{
	if (!OutlineComponent || !InteractionCollision)
	{
		return;
	}

	OutlineComponent->SetupTrigger(InteractionCollision);
	OutlineComponent->SetOverlapTriggerEnabled(true);
}

APDPlayerController* APDInteractableActorBase::ResolveInteractorController(AActor* Interactor) const
{
	const APawn* InteractingPawn = Cast<APawn>(Interactor);
	return InteractingPawn ? Cast<APDPlayerController>(InteractingPawn->GetController()) : nullptr;
}
