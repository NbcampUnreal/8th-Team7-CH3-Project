#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "PDBTTask_RotateToFaceTarget.generated.h"

class UBehaviorTreeComponent;

/**
 * BB 키 방향으로 "조준 잠금" 회전.
 *  - BlackboardKey 가 Object(AActor) 면 그 액터 위치, Vector 면 그 좌표를 향해 Yaw 보간.
 *    (사용 예: TargetActor → 발견 시 회전 / LastNoiseLocation → 소음 방향 회전)
 *  - 각도 차이가 AngleToleranceDegrees 이하가 되면 Succeeded.
 *  - TimeoutSeconds 초과 시 강제 Succeeded (BT 흐름 정체 방지).
 *  - 키가 비어있거나 타입 불일치면 Failed → 부모 Selector 가 다른 분기 평가.
 */
UCLASS(meta = (DisplayName = "PD Rotate To Face BB"))
class PROJECTD_API UPDBTTask_RotateToFaceTarget : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UPDBTTask_RotateToFaceTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	/** 회전 속도(deg/sec). 낮을수록 천천히 돈다. */
	UPROPERTY(EditAnywhere, Category = "PD|Rotation", meta = (ClampMin = "1.0"))
	float RotationSpeedDegPerSec = 360.f;

	/** 이 각도(절댓값) 이하면 정렬 완료로 간주. */
	UPROPERTY(EditAnywhere, Category = "PD|Rotation", meta = (ClampMin = "0.1", ClampMax = "45.0"))
	float AngleToleranceDegrees = 5.f;

	/** 회전 완료까지 최대 대기 시간(초). 0 이하면 무제한. */
	UPROPERTY(EditAnywhere, Category = "PD|Rotation", meta = (ClampMin = "0.0"))
	float TimeoutSeconds = 1.5f;

private:
	/** BB 키에서 회전 목표 위치 해석. Object/Vector 둘 다 지원. */
	bool ResolveTargetLocation(UBehaviorTreeComponent& OwnerComp, FVector& OutLocation) const;
};
