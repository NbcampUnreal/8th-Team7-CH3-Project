#include "Enemy/AI/BehaviorTree/PDBTTask_SetEnemyState.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "Characters/Base/PDEnemyBase.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"

UPDBTTask_SetEnemyState::UPDBTTask_SetEnemyState()
{
	NodeName = TEXT("PD Set Enemy State");

	EnemyStateKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(UPDBTTask_SetEnemyState, EnemyStateKey),
		StaticEnum<EPDEnemyState>());
	EnemyStateKey.SelectedKeyName = PDBTKeys::EnemyState;
}

EBTNodeResult::Type UPDBTTask_SetEnemyState::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!Pawn) return EBTNodeResult::Failed;

	if (APDEnemyBase* Enemy = Cast<APDEnemyBase>(Pawn))
	{
		Enemy->SetEnemyState(NewState);
	}

	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		BB->SetValueAsEnum(EnemyStateKey.SelectedKeyName, static_cast<uint8>(NewState));
	}

	return EBTNodeResult::Succeeded;
}
