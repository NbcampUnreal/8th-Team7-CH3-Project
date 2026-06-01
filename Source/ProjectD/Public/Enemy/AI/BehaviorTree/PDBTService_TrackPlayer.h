#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "PDBTService_TrackPlayer.generated.h"

/**
 * Elite 이상 적의 플레이어 Target designate Service.
 *
 * 동작:
 *  1) Acquire: 매 tick AcquireRange 안의 가장 가까운 player 폰을 찾아 BB.TargetActor 에 직접 지정.
 *     LOS 무시 — perception 의 첫 인식 latch 와 무관하게 거리만으로 designate.
 *  2) Maintain: TargetActor 가 이미 set 이고 player 가 acquire 범위 밖이어도, LoseRange 안이면 유지.
 *     → cover 이동으로 AI 가 멀어져도 target 이 꺼지지 않음 (hysteresis).
 *  3) Lose: TargetActor 가 LoseRange 밖이거나 invalid 면 stamp(LastSeenLocation/Direction)
 *     후 BB.TargetActor 클리어 → BT 가 Investigate 분기로 자연 전이.
 *
 * 부착 위치:
 *  - Combat 분기에만 부착 권장. Patrol 분기에 부착하면 시야 cone 무시하고 자동 aggro 됨.
 *  - 즉 첫 인식은 perception(sight) 가 담당, 본 Service 는 Combat 진입 이후 target 유지/획득 강화.
 */
UCLASS(meta = (DisplayName = "PD Track Player Target"))
class PROJECTD_API UPDBTService_TrackPlayer : public UBTService
{
	GENERATED_BODY()

public:
	UPDBTService_TrackPlayer();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	/** Acquire 거리(cm). 이 안에 들어온 가장 가까운 player 폰을 TargetActor 로 designate. */
	UPROPERTY(EditAnywhere, Category = "PD|Track", meta = (ClampMin = "0.0"))
	float AcquireRange = 2000.f;

	/** Lose 거리(cm). 한 번 designate 된 target 이 이 거리까지는 유지(hysteresis).
	 *  EQS Cover OuterRadius + player 가까이 다가오는 마진을 합친 값 이상 권장. */
	UPROPERTY(EditAnywhere, Category = "PD|Track", meta = (ClampMin = "0.0"))
	float LoseRange = 3500.f;

	/** LastSeenDirection 갱신 시 위치 델타 최소 크기(cm). 이보다 작으면 폰 velocity 폴백. */
	UPROPERTY(EditAnywhere, Category = "PD|Track", meta = (ClampMin = "0.0"))
	float MinDeltaForDirection = 5.f;
};

struct FPDTrackPlayerMemory
{
	FVector PrevTargetLoc = FVector::ZeroVector;
	bool bHadValidTarget  = false;
};
