#include "Enemy/AI/BehaviorTree/PDBTService_TrackLOSOcclusion.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "GameFramework/Actor.h"

UPDBTService_TrackLOSOcclusion::UPDBTService_TrackLOSOcclusion()
{
	NodeName = TEXT("PD Track LOS Occlusion");
	Interval = 0.2f;
	RandomDeviation = 0.05f;

	bNotifyTick = true;
	bTickIntervals = true;
}

void UPDBTService_TrackLOSOcclusion::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(PDBTKeys::TargetActor));
	const bool bHasLOS = BB->GetValueAsBool(PDBTKeys::HasLOSToTarget);

	float TimeSince = BB->GetValueAsFloat(PDBTKeys::TimeSinceLastLOS);
	if (Target && bHasLOS)
	{
		TimeSince = 0.f;
		// LOS 확보 동안 LastSeenLocation 을 최신화 — Chase 좌표 신뢰도 유지.
		BB->SetValueAsVector(PDBTKeys::LastSeenLocation, Target->GetActorLocation());
	}
	else if (Target)
	{
		TimeSince += DeltaSeconds;
	}
	else
	{
		TimeSince = 0.f;
	}
	BB->SetValueAsFloat(PDBTKeys::TimeSinceLastLOS, TimeSince);
}
