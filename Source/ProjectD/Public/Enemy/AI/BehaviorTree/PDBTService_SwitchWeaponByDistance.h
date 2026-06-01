#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "PDBTService_SwitchWeaponByDistance.generated.h"

/**
 * 매 tick BB.TargetActor 와의 거리를 측정해 APDEliteSoldier::SwitchWeaponByDistance 호출.
 *  - Pawn 이 APDEliteSoldier 가 아니면 no-op.
 *  - 타겟이 없으면 no-op (Combat 분기에만 부착되도록 BT 설계).
 *  - 임계값 자체는 APDEliteSoldier UPROPERTY 가 단일 진실 원천 — 본 Service 는 디스패처.
 */
UCLASS(meta = (DisplayName = "PD Switch Weapon By Distance"))
class PROJECTD_API UPDBTService_SwitchWeaponByDistance : public UBTService
{
	GENERATED_BODY()

public:
	UPDBTService_SwitchWeaponByDistance();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
