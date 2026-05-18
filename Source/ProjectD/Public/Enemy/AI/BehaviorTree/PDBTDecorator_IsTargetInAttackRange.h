#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "PDBTDecorator_IsTargetInAttackRange.generated.h"

/**
 * BB 의 TargetActor 와 Pawn 의 거리가 AttackRange 이내인지 매 평가 시점에 즉시 검사.
 *  - UPDBTService_UpdateCombatBB 의 IsTargetInRange(0.2s 폴링) 대비 0 지연.
 *  - BlackboardKey: TargetActor (Object). 키 변경 시 자동 옵저버 재평가.
 *  - 거리 자체는 매 틱 변하므로, 보조적으로 Service 가 BB 키를 갱신하는 흐름과 병행 사용 권장.
 *  - 사거리 출처 우선순위: RangeOverride(>0) → CombatComponent.AttackRange.
 */
UCLASS(meta = (DisplayName = "PD Is Target In Attack Range"))
class PROJECTD_API UPDBTDecorator_IsTargetInAttackRange : public UBTDecorator_BlackboardBase
{
	GENERATED_BODY()

public:
	UPDBTDecorator_IsTargetInAttackRange();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;

	/** 0 이하면 CombatComponent.AttackRange 사용. 양수면 이 값으로 강제(무기별 차등 등). */
	UPROPERTY(EditAnywhere, Category = "PD|Range", meta = (ClampMin = "0.0"))
	float RangeOverride = 0.f;
};
