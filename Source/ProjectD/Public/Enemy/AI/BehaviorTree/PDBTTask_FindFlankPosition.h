#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "PDBTTask_FindFlankPosition.generated.h"

/**
 * LOS 차단 시 측면 우회 좌표 탐색.
 *  - BlackboardKey(=TargetActor) 기준으로 Pawn->Target 방향을 좌/우 회전한 후보점들을 생성.
 *  - 각 후보를 NavMesh ProjectPoint 으로 보정 + 후보→Target LineTrace 로 LOS 확보 여부 검증.
 *  - 첫 LOS 확보 후보를 OutputLocationKey(=FlankLocation, Vector) 에 기록 → Succeeded.
 *  - 전부 실패 시 Failed → 부모 Selector 가 Chase 등 폴백 분기 평가.
 */
UCLASS(meta = (DisplayName = "PD Find Flank Position"))
class PROJECTD_API UPDBTTask_FindFlankPosition : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UPDBTTask_FindFlankPosition();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	/** 결과 저장 BB 키 (Vector). 기본 FlankLocation. MoveTo Task 가 이 키를 참조. */
	UPROPERTY(EditAnywhere, Category = "PD|Flank")
	FBlackboardKeySelector OutputLocationKey;

	/** Pawn 에서 측면 후보점까지 거리(cm). */
	UPROPERTY(EditAnywhere, Category = "PD|Flank", meta = (ClampMin = "100.0"))
	float FlankRadius = 600.f;

	/** 한쪽(좌/우) 당 샘플 수. 좌/우 합산 = 2 * NumSamplesPerSide. */
	UPROPERTY(EditAnywhere, Category = "PD|Flank", meta = (ClampMin = "1", ClampMax = "16"))
	int32 NumSamplesPerSide = 4;

	/** 정면(0°) 기준 측면 후보 시작 각도. 너무 작으면 정면 부근만 시도해서 LOS 재확보 실패. */
	UPROPERTY(EditAnywhere, Category = "PD|Flank", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float MinFlankAngleDegrees = 45.f;

	/** 정면(0°) 기준 측면 후보 끝 각도. 180 에 가까우면 후방까지 시도. */
	UPROPERTY(EditAnywhere, Category = "PD|Flank", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float MaxFlankAngleDegrees = 135.f;

	/** NavMesh ProjectPoint tolerance (cm). 너무 작으면 후보 자주 탈락. */
	UPROPERTY(EditAnywhere, Category = "PD|Flank", meta = (ClampMin = "10.0"))
	float NavProjectExtent = 200.f;

	/** LOS 검증 시 후보 위치 시점 보정 높이 (cm). 캐릭터 EyeHeight 근사. */
	UPROPERTY(EditAnywhere, Category = "PD|Flank", meta = (ClampMin = "0.0"))
	float TraceEyeHeight = 64.f;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};
