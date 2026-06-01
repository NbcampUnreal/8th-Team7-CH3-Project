#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "PDBTTask_PeekFireFromCover.generated.h"

class UEnvQuery;

/**
 * 엄폐물에서 settle → 랜덤 방향 strafe → LOS 확보 시 사격 → 원위치 복귀의 단일 latent Task.
 *
 * 흐름:
 *  1) ExecuteTask: 현재 폰 위치(=cover)를 OriginalLoc 으로 기록. SetFocus(Target). 상태=Settling.
 *  2) TickTask:
 *     - Settling: SettleDuration 대기 후 좌/우 랜덤 결정. ToTarget 기준 좌(또는 우) 방향으로
 *       PeekOffsetLeft(좌) / PeekOffsetRight(우) 만큼 떨어진 PeekSpot 계산 + MoveTo. 상태=Strafing.
 *       (좌측은 총구가 폰의 오른쪽에 있어 LOS 확보까지 더 노출 필요 → 거리 더 크게.)
 *     - Strafing: 매 tick HasLOS 검사. LOS 확보 즉시 StopMovement + SetPeeking(true) + 상태=Firing.
 *       LOS 미확보 상태로 PeekSpot 도착 시 그래도 발사 시도(fallback).
 *     - Firing: FireDuration 경과 OR Weapon->IsReloading() → SetPeeking(false) + MoveTo(OriginalLoc) + 상태=Returning.
 *     - Returning: OriginalLoc 도착 시 FinishLatentTask(Succeeded).
 *  3) AbortTask/OnTaskFinished: SetPeeking(false), ClearFocus, StopMovement.
 *
 * 사용자 사양:
 *  - 항상 우측 peek → 랜덤 좌/우 strafe 로 변경 (예측 가능성 제거).
 *  - "정면에 플레이어 보이면 사격" → LOS 확보를 게이트로 사용.
 *  - 좌측 노출 거리 ↑ — 총구가 우측에 있어 좌 strafe 시 더 많은 노출 필요.
 *  - 장전 시작 시 즉시 복귀.
 *
 * 부모 의존:
 *  - 사격은 APDEliteSoldier::SetPeeking(true/false) 가 들고 있는 풀오토 루프에 위임.
 *  - 장전 감지는 APDRangedWeaponBase::IsReloading() 폴링 (DYNAMIC_MULTICAST 델리게이트 lambda 바인딩 불가).
 */
UCLASS(meta = (DisplayName = "PD Peek Fire From Cover (LOS)"))
class PROJECTD_API UPDBTTask_PeekFireFromCover : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UPDBTTask_PeekFireFromCover();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	/** 우측 strafe 시 OriginalLoc 에서 PeekSpot 까지 거리(cm). 총구가 우측에 있어 짧아도 LOS 확보. */
	UPROPERTY(EditAnywhere, Category = "PD|PeekFire", meta = (ClampMin = "10.0"))
	float PeekOffsetRight = 100.f;

	/** 좌측 strafe 시 거리(cm). 총구가 우측에 있으므로 폰 중심이 cover 모서리 더 밖으로 나가야 함 → 길게. */
	UPROPERTY(EditAnywhere, Category = "PD|PeekFire", meta = (ClampMin = "10.0"))
	float PeekOffsetLeft = 180.f;

	/** 피크 전 cover 에서 대기할 시간(초). */
	UPROPERTY(EditAnywhere, Category = "PD|PeekFire", meta = (ClampMin = "0.0"))
	float SettleDuration = 1.5f;

	/** LOS 확보 후 사격 유지 시간(초). */
	UPROPERTY(EditAnywhere, Category = "PD|PeekFire", meta = (ClampMin = "0.1"))
	float FireDuration = 5.0f;

	/** Move 도착 판정 AcceptanceRadius(cm). */
	UPROPERTY(EditAnywhere, Category = "PD|PeekFire", meta = (ClampMin = "1.0"))
	float MoveAcceptanceRadius = 30.f;

	/** 전체 task safety timeout(초). Settling + Strafing + Firing + (Reposition) + Returning 전체 누적. */
	UPROPERTY(EditAnywhere, Category = "PD|PeekFire", meta = (ClampMin = "1.0"))
	float MaxTotalDuration = 20.f;

	/** true 면 Firing 중 weapon.IsReloading() 감지 시 즉시 Returning. */
	UPROPERTY(EditAnywhere, Category = "PD|PeekFire")
	bool bAbortOnReload = true;

	/** LOS line trace 시 폰/타겟 모두에 더할 Z offset(cm). 캐릭터 가슴 높이 근사. */
	UPROPERTY(EditAnywhere, Category = "PD|PeekFire", meta = (ClampMin = "0.0"))
	float LOSCheckHeight = 80.f;

	/** Firing 중 LOS 잃은 후 reposition 트리거까지의 grace 시간(초). 짧으면 잦은 reposition, 길면 무용한 사격 누적. */
	UPROPERTY(EditAnywhere, Category = "PD|PeekFire|Reposition", meta = (ClampMin = "0.0"))
	float LosLostTolerance = 0.4f;

	/** 사격 중 LOS 잃었을 때 가시 위치 산출용 EQS. nullptr 이면 reposition 안 함(cover 복귀). */
	UPROPERTY(EditAnywhere, Category = "PD|PeekFire|Reposition")
	TObjectPtr<UEnvQuery> RepositionQuery;
};

enum class EPDPeekFireState : uint8
{
	Settling,       // cover 도착 직후 SettleDuration 만큼 대기
	Strafing,       // PeekSpot 으로 측면 이동하며 매 tick LOS 검사
	Firing,         // LOS 확보, 사격 중
	Repositioning,  // 사격 중 LOS 잃어 EQS 결과로 가시 위치 이동 중
	Returning,      // 원위치로 복귀 중
};

struct FPDPeekFireTaskMemory
{
	FVector OriginalLoc       = FVector::ZeroVector;
	FVector PeekSpot          = FVector::ZeroVector;
	FVector RepositionTarget  = FVector::ZeroVector;
	float TotalElapsed   = 0.f;
	float SettleElapsed  = 0.f;
	float FireElapsed    = 0.f;
	float LosLostElapsed = 0.f;
	bool bGoLeft         = false;
	EPDPeekFireState State = EPDPeekFireState::Settling;
};
