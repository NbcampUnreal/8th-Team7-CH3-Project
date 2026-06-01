#include "Enemy/Animation/PDJuggernautLaunchNotify.h"

#include "Components/SkeletalMeshComponent.h"
#include "Enemy/Characters/PDJuggernaut.h"

void UPDJuggernautLaunchNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	// 런처 메시의 소유 액터 = 보스. 발사 시퀀스 시작(서버 가드는 보스 내부).
	if (APDJuggernaut* Boss = Cast<APDJuggernaut>(MeshComp->GetOwner()))
	{
		Boss->OnPattern2LaunchNotify();
	}
}

FString UPDJuggernautLaunchNotify::GetNotifyName_Implementation() const
{
	return TEXT("Juggernaut Missile Launch");
}
