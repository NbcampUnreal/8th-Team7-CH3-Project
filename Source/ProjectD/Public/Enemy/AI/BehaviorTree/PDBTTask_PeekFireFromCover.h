#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "PDBTTask_PeekFireFromCover.generated.h"

/**
 * 엄폐물에서 우측 피크 → 사격 → 원위치 복귀의 단일 latent Task.
 *
 * 흐름:
 *  1) ExecuteTask: 현재 폰 위치(=cover 위치)를 OriginalLoc 으로 기록.
 *     적(BlackboardKey=Target) 방향 기준 우측(Cross(ToTarget, Up))으로 PeekOffset 만큼 이동한 PeekSpot 계산.
 *     SetFocus(Target). AIController.MoveToLocation(PeekSpot). 상태=Peeking.
 *  2) TickTask:
 *     - Peeking 중 도착 → SetPeeking(true), 상태=Firing, FireTimer 시작.
 *     - Firing 중 FireDuration 경과 → SetPeeking(false), MoveToLocation(OriginalLoc), 상태=Returning.
 *     - Returning 중 도착 → FinishLatentTask(Succeeded).
 *  3) AbortTask/OnTaskFinished: SetPeeking(false), ClearFocus 정리.
 *
 * 사용자 사양:
 *  - 오른손잡이 → 항상 우측 피크. 우측 방향은 Soldier 자기 forward 기준이 아니라
 *    적과의 대치선(ToTarget) 기준 우측 — 자연스러운 피크 동작.
 *  - 우측 lean 애니메이션이 없으므로 위치 이동으로 표현.
 *  - 사격은 부모(APDSoldier::SetPeeking) 가 들고 있는 풀오토 루프에 위임.
 */
UCLASS(meta = (DisplayName = "PD Peek Fire From Cover (Right)"))
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
	/** 적 방향 기준 우측으로 빠져나갈 거리(cm). 캐릭터 캡슐 + 무기 길이 대비 적절히. */
	UPROPERTY(EditAnywhere, Category = "PD|PeekFire", meta = (ClampMin = "10.0"))
	float PeekOffset = 120.f;

	/** 피크 위치에서 사격을 유지할 시간(초). */
	UPROPERTY(EditAnywhere, Category = "PD|PeekFire", meta = (ClampMin = "0.1"))
	float FireDuration = 1.5f;

	/** Peek/Return 이동 시 MoveTo 의 AcceptanceRadius(cm). 너무 작으면 도착 판정 지연. */
	UPROPERTY(EditAnywhere, Category = "PD|PeekFire", meta = (ClampMin = "1.0"))
	float MoveAcceptanceRadius = 30.f;

	/** 전체 Task 가 이 시간을 넘으면 강제 종료(Failed). 이동 막힘 등 대비 safety. */
	UPROPERTY(EditAnywhere, Category = "PD|PeekFire", meta = (ClampMin = "1.0"))
	float MaxTotalDuration = 6.f;
};

enum class EPDPeekFireState : uint8
{
	Peeking,    // PeekSpot 으로 이동 중
	Firing,     // 피크 위치 도착, 사격 중
	Returning,  // 원위치로 복귀 중
};

struct FPDPeekFireTaskMemory
{
	FVector OriginalLoc = FVector::ZeroVector;
	FVector PeekSpot    = FVector::ZeroVector;
	float TotalElapsed  = 0.f;
	float FireElapsed   = 0.f;
	EPDPeekFireState State = EPDPeekFireState::Peeking;
};
