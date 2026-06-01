#include "Component/PDInteractionComponent.h"

#include "Interfaces/PDInteractable.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "CollisionShape.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"

namespace
{
	bool ShouldIgnoreInteractTarget(const AActor* OwnerActor, const AActor* CandidateActor)
	{
		if (!OwnerActor || !CandidateActor || CandidateActor == OwnerActor)
		{
			return true;
		}

		return CandidateActor->GetOwner() == OwnerActor ||
			CandidateActor->GetAttachParentActor() == OwnerActor;
	}
}

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
	CachedTarget.Reset();

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

		ApplyInteractTargetChange(nullptr);
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

	if (!OwnerActor || !GetWorld())
	{
		return nullptr;
	}

	const FVector Start = OwnerActor->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = Start + OwnerActor->GetActorForwardVector() * InteractDistance;

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PDInteractionTrace), false, OwnerActor);
	Params.AddIgnoredActor(OwnerActor);
	TArray<AActor*> AttachedActors;
	OwnerActor->GetAttachedActors(AttachedActors);
	for (AActor* AttachedActor : AttachedActors)
	{
		Params.AddIgnoredActor(AttachedActor);
	}
	const FCollisionShape Shape = FCollisionShape::MakeSphere(80.f);

	GetWorld()->SweepMultiByChannel(
		Hits,
		Start,
		End,
		FQuat::Identity,
		TraceChannel,
		Shape,
		Params
	);

	GetWorld()->SweepMultiByChannel(
		Hits,
		Start,
		Start + FVector(0.f, 0.f, 1.f),
		FQuat::Identity,
		TraceChannel,
		FCollisionShape::MakeSphere(InteractDistance),
		Params
	);

	AActor* ClosestInteractable = nullptr;
	float ClosestDistanceSq = TNumericLimits<float>::Max();

	auto ConsiderInteractable = [&](AActor* HitActor, const FVector& HitLocation)
	{
		if (ShouldIgnoreInteractTarget(OwnerActor, HitActor))
		{
			return;
		}

		if (!HitActor->GetClass()->ImplementsInterface(UPDInteractable::StaticClass()))
		{
			return;
		}

		// 현재 상호작용 불가(적, 비다운 플레이어 등)면 대상에서 제외 → 프롬프트도 안 뜸.
		if (!IPDInteractable::Execute_CanInteract(HitActor, OwnerActor))
		{
			return;
		}

		const float DistanceSq = FVector::DistSquared(Start, HitLocation);

		if (DistanceSq < ClosestDistanceSq)
		{
			ClosestDistanceSq = DistanceSq;
			ClosestInteractable = HitActor;
		}
	};

	for (const FHitResult& Hit : Hits)
	{
		ConsiderInteractable(Hit.GetActor(), Hit.ImpactPoint);
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		Start,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(InteractDistance),
		Params
	);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (AActor* OverlapActor = Overlap.GetActor())
		{
			ConsiderInteractable(OverlapActor, OverlapActor->GetActorLocation());
		}
	}

	return ClosestInteractable;
}

void UPDInteractionComponent::PollTarget()
{
	ApplyInteractTargetChange(FindInteractTarget());
}

void UPDInteractionComponent::ApplyInteractTargetChange(AActor* NewTarget)
{
	AActor* OldTarget = CachedTarget.Get();

	if (NewTarget == OldTarget)
	{
		return;
	}

	CachedTarget = NewTarget;

	OnInteractTargetChanged.Broadcast(NewTarget);
}
