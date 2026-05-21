#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "PDBTService_TrackPlayer.generated.h"

/**
 * Elite 이상 적의 플레이어 위치 추적.
 *  - Perception/LOS 우회 — 범위 내라면 정확한 현재 위치를 BB 에 저장.
 *  - 매 Tick(Interval) 주기로 PlayerPawn 거리 검사.
 *    - 범위 내: BB.TrackedPlayerLocation = Player->GetActorLocation(), bHasTrackedPlayer = true
 *    - 범위 밖: bHasTrackedPlayer = false (TrackedPlayerLocation 은 마지막값 유지)
 *  - 멀티 플레이어 대응: 가장 가까운 플레이어 폰을 추적.
 *
 * 디자인 의도:
 *  - 사용자가 명시한 "엘리트부터는 범위 내 플레이어 위치를 아예 인식".
 *  - 일반 Soldier 는 본 Service 미부착 — perception 만 사용.
 */
UCLASS(meta = (DisplayName = "PD Track Player Location"))
class PROJECTD_API UPDBTService_TrackPlayer : public UBTService
{
	GENERATED_BODY()

public:
	UPDBTService_TrackPlayer();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	/** 추적 최대 거리(cm). 이 거리 밖이면 인식 X. */
	UPROPERTY(EditAnywhere, Category = "PD|Track", meta = (ClampMin = "0.0"))
	float TrackingRange = 5000.f;
};
