#include "Enemy/AI/BehaviorTree/PDBTService_TrackPlayer.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UPDBTService_TrackPlayer::UPDBTService_TrackPlayer()
{
	NodeName = TEXT("PD Track Player Target");
	bNotifyTick = true;
	Interval = 0.2f;
	RandomDeviation = 0.05f;
}

uint16 UPDBTService_TrackPlayer::GetInstanceMemorySize() const
{
	return sizeof(FPDTrackPlayerMemory);
}

namespace
{
	// LastSeenDirection 갱신 헬퍼. 위치 델타 우선, 너무 작으면 velocity 폴백.
	void UpdateDirection(UBlackboardComponent* BB, FPDTrackPlayerMemory* Mem,
		const AActor* Target, float MinDelta)
	{
		if (!BB || !Mem || !Target) return;
		const FVector NowLoc = Target->GetActorLocation();

		if (Mem->bHadValidTarget)
		{
			const FVector Delta = NowLoc - Mem->PrevTargetLoc;
			if (Delta.SizeSquared2D() >= MinDelta * MinDelta)
			{
				BB->SetValueAsVector(PDBTKeys::LastSeenDirection, Delta.GetSafeNormal2D());
				return;
			}
			const FVector Vel = Target->GetVelocity();
			if (!Vel.IsNearlyZero())
			{
				BB->SetValueAsVector(PDBTKeys::LastSeenDirection, Vel.GetSafeNormal2D());
			}
		}
	}
}

void UPDBTService_TrackPlayer::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* SelfPawn = AI ? AI->GetPawn() : nullptr;
	UWorld* World = GetWorld();
	FPDTrackPlayerMemory* Mem = reinterpret_cast<FPDTrackPlayerMemory*>(NodeMemory);

	if (!BB || !SelfPawn || !World || !Mem) return;

	const FVector SelfLoc = SelfPawn->GetActorLocation();
	const float AcquireSq = AcquireRange * AcquireRange;
	const float LoseSq    = LoseRange    * LoseRange;

	// 1) AcquireRange 안의 가장 가까운 player 폰 탐색 (멀티 대응).
	APawn* ClosestPlayer = nullptr;
	float ClosestDistSq = TNumericLimits<float>::Max();

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;

		APawn* PlayerPawn = PC->GetPawn();
		if (!PlayerPawn || PlayerPawn == SelfPawn) continue;

		const float DistSq = FVector::DistSquared(SelfLoc, PlayerPawn->GetActorLocation());
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			ClosestPlayer = PlayerPawn;
		}
	}

	// 2) Acquire — 새 target 설정 / 기존 갱신.
	if (ClosestPlayer && ClosestDistSq <= AcquireSq)
	{
		BB->SetValueAsObject(PDBTKeys::TargetActor, ClosestPlayer);
		BB->SetValueAsVector(PDBTKeys::LastSeenLocation, ClosestPlayer->GetActorLocation());
		BB->SetValueAsBool(PDBTKeys::bHasTrackedPlayer, true);

		UpdateDirection(BB, Mem, ClosestPlayer, MinDeltaForDirection);

		Mem->PrevTargetLoc   = ClosestPlayer->GetActorLocation();
		Mem->bHadValidTarget = true;
		return;
	}

	// 3) Maintain — acquire 범위 밖이지만 기존 TargetActor 가 LoseRange 안이면 유지.
	AActor* CurTarget = Cast<AActor>(BB->GetValueAsObject(PDBTKeys::TargetActor));
	if (IsValid(CurTarget))
	{
		const FVector CurLoc = CurTarget->GetActorLocation();
		const float CurDistSq = FVector::DistSquared(SelfLoc, CurLoc);

		if (CurDistSq <= LoseSq)
		{
			// hysteresis 영역 — target 유지, last seen 만 갱신.
			BB->SetValueAsVector(PDBTKeys::LastSeenLocation, CurLoc);
			BB->SetValueAsBool(PDBTKeys::bHasTrackedPlayer, true);

			UpdateDirection(BB, Mem, CurTarget, MinDeltaForDirection);

			Mem->PrevTargetLoc   = CurLoc;
			Mem->bHadValidTarget = true;
			return;
		}

		// 4) Lose — LoseRange 밖 OR invalid. stamp 후 클리어.
		BB->SetValueAsVector(PDBTKeys::LastSeenLocation, CurLoc);
		BB->ClearValue(PDBTKeys::TargetActor);
	}

	BB->SetValueAsBool(PDBTKeys::bHasTrackedPlayer, false);
	Mem->bHadValidTarget = false;
}
