#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PDJuggernautLaunchNotify.generated.h"

/**
 * Juggernaut 패턴2(미사일 스웜) 발사 트리거.
 * 런처 애님의 발사각 도달 프레임(예: 14)에 배치 → 소유 Juggernaut 의 발사 시퀀스를 시작.
 * 런처 메시 단일노드 재생에서도 발화. 게임플레이는 서버에서만(보스가 HasAuthority 가드).
 */
UCLASS(meta = (DisplayName = "PD Juggernaut Missile Launch"))
class PROJECTD_API UPDJuggernautLaunchNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
