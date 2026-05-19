#include "Component/PDInteractionComponent.h"

#include "Interfaces/PDInteractable.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "TimerManager.h"
UPDInteractionComponent::UPDInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	// 멀티: Controller가 BeginPlay 이후 늦게 replicate되는 경우(클라이언트 폰)를 위해
	// 변경 시점마다 폴링 상태를 재평가. 첫 호출은 EvaluatePollingState로 즉시 평가.
	OwnerPawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UPDInteractionComponent::HandleOwnerControllerChanged);

	EvaluatePollingState();
}

void UPDInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UPDInteractionComponent::HandleOwnerControllerChanged);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UPDInteractionComponent::HandleOwnerControllerChanged(APawn* /*InPawn*/, AController* /*OldController*/, AController* /*NewController*/)
{
	EvaluatePollingState();
}

void UPDInteractionComponent::EvaluatePollingState()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	UWorld* World = GetWorld();
	if (!OwnerPawn || !World)
	{
		return;
	}

	const bool bShouldPoll = OwnerPawn->IsLocallyControlled();
	const bool bIsPolling = World->GetTimerManager().IsTimerActive(PollTimerHandle);

	if (bShouldPoll && !bIsPolling)
	{
		World->GetTimerManager().SetTimer(
			PollTimerHandle,
			this,
			&UPDInteractionComponent::PollTarget,
			PollInterval,
			true,
			0.f);
	}
	else if (!bShouldPoll && bIsPolling)
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);

		// 로컬 제어를 잃은 경우 HUD 프롬프트가 잔존하지 않도록 타겟 클리어.
		if (CachedTarget.IsValid())
		{
			CachedTarget.Reset();
			OnInteractTargetChanged.Broadcast(nullptr);
		}
	}
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
	UWorld* World = GetWorld();

	if (!OwnerActor || !World)
	{
		return nullptr;
	}

	const FVector Start = OwnerActor->GetActorLocation() + FVector(0.f, 0.f, 50.f);

	TArray<AActor*> OverlappingActors;
	OwnerActor->GetOverlappingActors(OverlappingActors);

	if (AActor* OverlapTarget = ChooseClosestInteractable(OverlappingActors, Start))
	{
		return OverlapTarget;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams OverlapParams(SCENE_QUERY_STAT(PDInteractionOverlap), false, OwnerActor);
	OverlapParams.AddIgnoredActor(OwnerActor);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

	const bool bHasOverlap = World->OverlapMultiByObjectType(
		OverlapResults,
		Start,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(OverlapProbeRadius),
		OverlapParams);

	if (bHasOverlap)
	{
		TArray<AActor*> ProbeActors;

		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* ResultActor = Result.GetActor();

			if (ResultActor && ResultActor != OwnerActor)
			{
				ProbeActors.AddUnique(ResultActor);
			}
		}

		if (AActor* ProbeTarget = ChooseClosestInteractable(ProbeActors, Start))
		{
			return ProbeTarget;
		}
	}

	const FVector End = Start + OwnerActor->GetActorForwardVector() * InteractDistance;

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PDInteractionTrace), false, OwnerActor);
	Params.AddIgnoredActor(OwnerActor);
	const FCollisionShape Shape = FCollisionShape::MakeSphere(80.f);

	const bool bHit = World->SweepMultiByChannel(
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

	TArray<AActor*> HitActors;

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();

		if (HitActor && HitActor != OwnerActor)
		{
			HitActors.AddUnique(HitActor);
		}
	}

	return ChooseClosestInteractable(HitActors, Start);
}

AActor* UPDInteractionComponent::ChooseClosestInteractable(const TArray<AActor*>& Candidates, const FVector& FromLocation) const
{
	AActor* OwnerActor = GetOwner();
	AActor* ClosestInteractable = nullptr;
	float ClosestDistanceSq = TNumericLimits<float>::Max();

	for (AActor* Candidate : Candidates)
	{
		if (!Candidate || Candidate == OwnerActor)
		{
			continue;
		}

		if (!Candidate->GetClass()->ImplementsInterface(UPDInteractable::StaticClass()))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(FromLocation, Candidate->GetActorLocation());

		if (DistanceSq < ClosestDistanceSq)
		{
			ClosestDistanceSq = DistanceSq;
			ClosestInteractable = Candidate;
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