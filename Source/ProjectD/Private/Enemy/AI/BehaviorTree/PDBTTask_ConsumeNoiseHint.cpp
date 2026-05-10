#include "Enemy/AI/BehaviorTree/PDBTTask_ConsumeNoiseHint.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/Components/PDCombatComponent.h"

UPDBTTask_ConsumeNoiseHint::UPDBTTask_ConsumeNoiseHint()
{
	NodeName = TEXT("PD Consume Noise Hint");
}

EBTNodeResult::Type UPDBTTask_ConsumeNoiseHint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (UPDCombatComponent* Combat = Pawn ? Pawn->FindComponentByClass<UPDCombatComponent>() : nullptr)
	{
		Combat->ClearNoiseHint();
	}

	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		BB->SetValueAsBool(PDBTKeys::HasNoiseHint, false);
	}

	return EBTNodeResult::Succeeded;
}
