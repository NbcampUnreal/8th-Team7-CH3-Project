#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "PDBTTask_FireAtTarget.generated.h"

/**
 * 매 Tick 타겟 방향으로 Pawn 을 회전 + 정렬되면 RequestAttack() 시도.
 *  - 정렬(=Yaw 어긋남 ≤ AlignmentToleranceDegrees) 전까지는 발사 보류 → 첫 발부터 명중률 확보.
 *  - 플레이어가 움직이면 자동 재정렬 후 재발사 (Task 종료 없이 Tick 내 재처리).
 *  - 타겟이 사라지면 Failed → 부모 Selector 가 다른 분기 평가.
 *  - Focus 는 EnterTask 에서 한 번만 설정, AbortTask/Finished 에서 해제 (애님 aim layer 가 사용할 수 있도록).
 */
UCLASS(meta = (DisplayName = "PD Fire At Target"))
class PROJECTD_API UPDBTTask_FireAtTarget : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UPDBTTask_FireAtTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

protected:
	/** SetFocus 사용 여부. 애님 aim layer 가 control rotation 을 참조하면 켜두는 것이 권장. */
	UPROPERTY(EditAnywhere, Category = "PD|Fire")
	bool bSetFocusOnTarget = true;

	/** Pawn 을 직접 회전시켜 타겟 정렬할지. false 면 SetFocus + 캐릭터 bUseControllerRotationYaw 에 의존. */
	UPROPERTY(EditAnywhere, Category = "PD|Fire|Alignment")
	bool bRotatePawnToTarget = true;

	/** 회전 속도(deg/sec). RotateToFaceTarget 와 동일 디폴트. */
	UPROPERTY(EditAnywhere, Category = "PD|Fire|Alignment", meta = (ClampMin = "1.0", EditCondition = "bRotatePawnToTarget"))
	float RotationSpeedDegPerSec = 360.f;

	/** 발사 허용 Yaw 오차(deg). 이 값 안일 때만 RequestAttack. */
	UPROPERTY(EditAnywhere, Category = "PD|Fire|Alignment", meta = (ClampMin = "0.1", ClampMax = "45.0"))
	float AlignmentToleranceDegrees = 5.f;
};
