#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "PDBTService_TrackLOSOcclusion.generated.h"

/**
 * LOS 차단 누적 시간 추적.
 *  - HasLOSToTarget(=true) 시 TimeSinceLastLOS=0, LastSeenLocation 을 타겟 현재 위치로 갱신.
 *  - HasLOSToTarget(=false) 시 TimeSinceLastLOS 누적.
 *
 * 선행: 동일 노드에 UPDBTService_UpdateCombatBB 가 먼저 붙어 HasLOSToTarget 키를 갱신해야 함.
 */
UCLASS(meta = (DisplayName = "PD Track LOS Occlusion"))
class PROJECTD_API UPDBTService_TrackLOSOcclusion : public UBTService
{
	GENERATED_BODY()

public:
	UPDBTService_TrackLOSOcclusion();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
