#include "Component/PDInteractionComponent.h"

#include "Interfaces/PDInteractable.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "CollisionShape.h"
#include "TimerManager.h"

UPDInteractionComponent::UPDInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PollTimerHandle,
			this,
			&UPDInteractionComponent::PollTarget,
			PollInterval,
			true,
			0.f);
	}
}

void UPDInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
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

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PDInteractionTrace), false, OwnerActor);
	Params.AddIgnoredActor(OwnerActor);
	const FCollisionShape Shape = FCollisionShape::MakeSphere(80.f);

	const bool bHit = GetWorld()->SweepMultiByChannel(
		Hits,
		Start,
		End,
		FQuat::Identity,
		TraceChannel,
		Shape,
		Params
	);

	if (!bHit)
	{
		return nullptr;
	}

	AActor* ClosestInteractable = nullptr;
	float ClosestDistanceSq = TNumericLimits<float>::Max();

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();

		if (!HitActor || HitActor == OwnerActor)
		{
			continue;
		}

		if (!HitActor->GetClass()->ImplementsInterface(UPDInteractable::StaticClass()))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(Start, Hit.ImpactPoint);

		if (DistanceSq < ClosestDistanceSq)
		{
			ClosestDistanceSq = DistanceSq;
			ClosestInteractable = HitActor;
		}
	}

	return ClosestInteractable;
}

void UPDInteractionComponent::PollTarget()
{
	AActor* NewTarget = FindInteractTarget();
	AActor* OldTarget = CachedTarget.Get();

	if (NewTarget == OldTarget)
	{
		return;
	}

	CachedTarget = NewTarget;
	OnInteractTargetChanged.Broadcast(NewTarget);
}