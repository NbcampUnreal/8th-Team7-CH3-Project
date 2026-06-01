#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "PDBTService_NotifyAlliesPeriodic.generated.h"

/**
 * Combat 분기 위에 부착해 NotifyInterval(기본 5초) 마다 squad 통보를 반복 호출.
 *  - 단발 UPDBTTask_NotifyAllies 는 진입 시 1회만 발화 — 주기형은 본 Service.
 *  - PDCombatComponent.NotifyAlliesCooldown 가 자체 throttle 을 가져
 *    interval 더 짧게 잡아도 ally 가 영구 재설정되진 않음 — 안전.
 */
UCLASS(meta = (DisplayName = "PD Notify Allies Periodic"))
class PROJECTD_API UPDBTService_NotifyAlliesPeriodic : public UBTService
{
	GENERATED_BODY()

public:
	UPDBTService_NotifyAlliesPeriodic();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	/** 통보 주기(초). */
	UPROPERTY(EditAnywhere, Category = "PD|Squad", meta = (ClampMin = "0.1"))
	float NotifyInterval = 5.f;

	/** 통보 반경(cm). */
	UPROPERTY(EditAnywhere, Category = "PD|Squad", meta = (ClampMin = "0.0"))
	float NotifyRadius = 1500.f;
};

struct FPDNotifyPeriodicMemory
{
	float Accumulated = 0.f;
};
