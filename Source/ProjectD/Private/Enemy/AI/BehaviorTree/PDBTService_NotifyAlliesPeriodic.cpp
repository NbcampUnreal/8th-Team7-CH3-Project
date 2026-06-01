#include "Enemy/AI/BehaviorTree/PDBTService_NotifyAlliesPeriodic.h"

#include "AIController.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "GameFramework/Pawn.h"

UPDBTService_NotifyAlliesPeriodic::UPDBTService_NotifyAlliesPeriodic()
{
	NodeName = TEXT("PD Notify Allies Periodic");
	bNotifyTick = true;
	bNotifyBecomeRelevant = true;
	// 누적 정밀도 충분. 5s/1s = 5 tick 차이 정도 무시 가능.
	Interval = 1.0f;
	RandomDeviation = 0.1f;
}

uint16 UPDBTService_NotifyAlliesPeriodic::GetInstanceMemorySize() const
{
	return sizeof(FPDNotifyPeriodicMemory);
}

void UPDBTService_NotifyAlliesPeriodic::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);
	// Combat 분기 진입 즉시 1회 통보. 단발 NotifyAllies Task 대체.
	if (FPDNotifyPeriodicMemory* Mem = reinterpret_cast<FPDNotifyPeriodicMemory*>(NodeMemory))
	{
		Mem->Accumulated = NotifyInterval;
	}
}

void UPDBTService_NotifyAlliesPeriodic::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FPDNotifyPeriodicMemory* Mem = reinterpret_cast<FPDNotifyPeriodicMemory*>(NodeMemory);
	if (!Mem) return;

	Mem->Accumulated += DeltaSeconds;
	if (Mem->Accumulated < NotifyInterval) return;
	Mem->Accumulated = 0.f;

	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	UPDCombatComponent* Combat = Pawn ? Pawn->FindComponentByClass<UPDCombatComponent>() : nullptr;
	if (!Combat) return;

	AActor* Target = Combat->GetCurrentTarget();
	if (!Target) return;

	Combat->NotifyAlliesInRadius(NotifyRadius, Target);
}
