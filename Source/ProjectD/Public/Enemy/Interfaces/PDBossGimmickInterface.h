#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PDBossGimmickInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPDBossGimmickInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 보스 전용 기믹(Phase 전환, 페이즈별 능력 활성화 등) 진입점.
 * 현재는 stub — Boss 캐릭터 추가 시점에 함수 정의.
 * 일반 적은 본 인터페이스를 구현하지 않아 ISP 위배 방지.
 */
class PROJECTD_API IPDBossGimmickInterface
{
	GENERATED_BODY()
};
