#include "Enemy/AI/BehaviorTree/PDBTTask_NotifyAllies.h"

#include "AIController.h"
#include "Enemy/Components/PDCombatComponent.h"

UPDBTTask_NotifyAllies::UPDBTTask_NotifyAllies()
{
	NodeName = TEXT("PD Notify Allies");
}

EBTNodeResult::Type UPDBTTask_NotifyAllies::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	UPDCombatComponent* Combat = Pawn ? Pawn->FindComponentByClass<UPDCombatComponent>() : nullptr;
	if (!Combat) return EBTNodeResult::Failed;

	AActor* Target = Combat->GetCurrentTarget();
	if (!Target) return EBTNodeResult::Failed;

	Combat->NotifyAlliesInRadius(NotifyRadius, Target);
	return EBTNodeResult::Succeeded;
}
