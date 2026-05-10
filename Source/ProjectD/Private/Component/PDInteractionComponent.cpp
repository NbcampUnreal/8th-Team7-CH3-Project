#include "Component/PDInteractionComponent.h"

#include "Interfaces/PDInteractable.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "CollisionShape.h"

UPDInteractionComponent::UPDInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDInteractionComponent::Interact()
{
	AActor* TargetActor = FindInteractTarget();

	if (!TargetActor)
	{
		return;
	}

	if (TargetActor->GetClass()->ImplementsInterface(UPDInteractable::StaticClass()))
	{
		IPDInteractable::Execute_Interact(TargetActor, GetOwner());
	}
}

AActor* UPDInteractionComponent::FindInteractTarget() const
{
	AActor* OwnerActor = GetOwner();

	if (!OwnerActor || !GetWorld())
	{
		return nullptr;
	}

	const FVector Start = OwnerActor->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = Start + OwnerActor->GetActorForwardVector() * InteractDistance;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PDInteractionTrace), false, OwnerActor);
	const FCollisionShape Shape = FCollisionShape::MakeSphere(80.f);

	const bool bHit = GetWorld()->SweepSingleByChannel(
		Hit,
		Start,
		End,
		FQuat::Identity,
		TraceChannel,
		Shape,
		Params
	);

	return bHit ? Hit.GetActor() : nullptr;
}
