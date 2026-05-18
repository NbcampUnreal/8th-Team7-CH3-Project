#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "PDBTDecorator_IsAwayFromHome.generated.h"

/**
 * Pawn 의 현재 위치 ↔ BB.HomeLocation 거리가 AwayDistance 보다 크면 true.
 *  - Return Home 분기 게이트로 사용 — 집 근처일 땐 false → 분기 skip.
 *  - BlackboardKey: HomeLocation (Vector). 디자이너가 다른 키로 변경 가능.
 *  - Pawn 위치는 매 평가 시점에 즉시 계산 — 거리 변화 자체로 옵저버 트리거되지 않으므로,
 *    UpdateCombatBB 같은 Service 가 BB 키를 갱신해주는 흐름과 자연스럽게 결합.
 */
UCLASS(meta = (DisplayName = "PD Is Away From Home"))
class PROJECTD_API UPDBTDecorator_IsAwayFromHome : public UBTDecorator_BlackboardBase
{
	GENERATED_BODY()

public:
	UPDBTDecorator_IsAwayFromHome();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;

	/** 이 거리를 넘으면 "집 떠남" 판정 (cm). */
	UPROPERTY(EditAnywhere, Category = "PD|Home", meta = (ClampMin = "0.0"))
	float AwayDistance = 300.f;
};
