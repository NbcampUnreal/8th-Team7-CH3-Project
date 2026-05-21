#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "PDBTTask_StrafeAroundTarget.generated.h"

UENUM()
enum class EPDStrafeDirection : uint8
{
	Random UMETA(DisplayName = "Random"),
	Left   UMETA(DisplayName = "Left"),
	Right  UMETA(DisplayName = "Right"),
};

/**
 * 적을 바라본 채 좌/우 측면으로 사이드스텝(strafe).
 *  - 진입 시 좌/우 1회 결정 (Random 옵션은 50/50).
 *  - 매 틱 Pawn->Target 에 수직인 lateral 방향으로 AddMovementInput.
 *  - DesiredRange 기준으로 너무 가까우면 후퇴 입력, 멀면 전진 입력 추가 — 적 중심 원호 유지.
 *  - AAIController::SetFocus 로 적 조준 잠금 — 종료 시 ClearFocus.
 *  - Duration 경과 시 Succeeded.
 *  - Target 소실/Pawn 무효 → Failed.
 *
 * 캐릭터 회전 주의:
 *  - 본 Task 는 SetFocus 만 호출 — 폰 BP 의 CharacterMovement 가
 *    bOrientRotationToMovement=true 면 회전이 이동 방향을 따라가 SetFocus 와 충돌.
 *    enemy BP 에서 bOrientRotationToMovement=false + bUseControllerDesiredRotation=true 권장.
 */
UCLASS(meta = (DisplayName = "PD Strafe Around Target"))
class PROJECTD_API UPDBTTask_StrafeAroundTarget : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UPDBTTask_StrafeAroundTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	/** 좌/우 선택. Random 은 50/50. */
	UPROPERTY(EditAnywhere, Category = "PD|Strafe")
	EPDStrafeDirection Direction = EPDStrafeDirection::Random;

	/** Task 지속 시간(초). 경과 시 Succeeded. */
	UPROPERTY(EditAnywhere, Category = "PD|Strafe", meta = (ClampMin = "0.1"))
	float Duration = 2.f;

	/** 적과 유지하려는 목표 거리(cm). */
	UPROPERTY(EditAnywhere, Category = "PD|Strafe", meta = (ClampMin = "0.0"))
	float DesiredRange = 600.f;

	/** DesiredRange 허용 오차(cm). 이 범위 안이면 거리 보정 입력 없음. */
	UPROPERTY(EditAnywhere, Category = "PD|Strafe", meta = (ClampMin = "0.0"))
	float DistanceTolerance = 100.f;

	/** 측면 이동 입력 스케일(0~1). MaxWalkSpeed 대비 비율. */
	UPROPERTY(EditAnywhere, Category = "PD|Strafe", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LateralInputScale = 1.f;

	/** 거리 보정 입력 스케일(0~1). 측면 이동과 합쳐져도 1을 넘지 않도록 작게 권장. */
	UPROPERTY(EditAnywhere, Category = "PD|Strafe", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DistanceCorrectScale = 0.5f;
};

struct FPDStrafeTaskMemory
{
	float ElapsedTime = 0.f;
	// +1 = 적 기준 왼쪽, -1 = 오른쪽. CrossProduct(ToTarget, Up) 의 부호.
	int32 SideSign = 1;
};
