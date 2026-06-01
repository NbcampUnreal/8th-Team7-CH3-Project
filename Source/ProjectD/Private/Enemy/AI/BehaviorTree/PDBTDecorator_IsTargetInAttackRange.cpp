#include "Enemy/AI/BehaviorTree/PDBTDecorator_IsTargetInAttackRange.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "GameFramework/Pawn.h"

UPDBTDecorator_IsTargetInAttackRange::UPDBTDecorator_IsTargetInAttackRange()
{
	NodeName = TEXT("PD Is Target In Attack Range");

	// 기본 입력: TargetActor (Object). 디자이너가 BB 에셋에서 다른 키로 바꿔도 됨.
	BlackboardKey.SelectedKeyName = PDBTKeys::TargetActor;
}

bool UPDBTDecorator_IsTargetInAttackRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/) const
{
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return false;

	const AActor* Target = Cast<AActor>(BB->GetValueAsObject(BlackboardKey.SelectedKeyName));
	if (!Target) return false;

	const AAIController* AICon = OwnerComp.GetAIOwner();
	const APawn* Pawn = AICon ? AICon->GetPawn() : nullptr;
	if (!Pawn) return false;

	float Range = RangeOverride;
	if (Range <= 0.f)
	{
		const UPDCombatComponent* Combat = Pawn->FindComponentByClass<UPDCombatComponent>();
		Range = Combat ? Combat->GetAttackRange() : 0.f;
	}
	if (Range <= 0.f) return false;

	const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), Target->GetActorLocation());
	return DistSq <= (Range * Range);
}

FString UPDBTDecorator_IsTargetInAttackRange::GetStaticDescription() const
{
	if (RangeOverride > 0.f)
	{
		return FString::Printf(TEXT("Target in attack range (override=%.0f)"), RangeOverride);
	}
	return TEXT("Target in attack range (CombatComponent)");
}
