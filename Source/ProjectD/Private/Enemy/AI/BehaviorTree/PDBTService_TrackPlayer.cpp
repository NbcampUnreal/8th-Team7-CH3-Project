#include "Enemy/AI/BehaviorTree/PDBTService_TrackPlayer.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UPDBTService_TrackPlayer::UPDBTService_TrackPlayer()
{
	NodeName = TEXT("PD Track Player Location");
	bNotifyTick = true;
	// 매 0.2초 정도면 EQS/이동 갱신엔 충분. 디자이너가 Interval/RandomDeviation 으로 조정.
	Interval = 0.2f;
	RandomDeviation = 0.05f;
}

void UPDBTService_TrackPlayer::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* SelfPawn = AI ? AI->GetPawn() : nullptr;
	UWorld* World = GetWorld();

	if (!BB || !SelfPawn || !World)
	{
		if (BB) BB->SetValueAsBool(PDBTKeys::bHasTrackedPlayer, false);
		return;
	}

	const FVector SelfLoc = SelfPawn->GetActorLocation();
	const float RangeSq = TrackingRange * TrackingRange;

	// 가장 가까운 PlayerController 의 폰을 추적. 멀티 대응.
	APawn* ClosestPlayerPawn = nullptr;
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
			ClosestPlayerPawn = PlayerPawn;
		}
	}

	if (ClosestPlayerPawn && ClosestDistSq <= RangeSq)
	{
		BB->SetValueAsVector(PDBTKeys::TrackedPlayerLocation, ClosestPlayerPawn->GetActorLocation());
		BB->SetValueAsBool(PDBTKeys::bHasTrackedPlayer, true);
	}
	else
	{
		BB->SetValueAsBool(PDBTKeys::bHasTrackedPlayer, false);
	}
}
