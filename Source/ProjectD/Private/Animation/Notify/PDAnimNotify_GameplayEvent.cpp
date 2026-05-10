#include "Animation/Notify/PDAnimNotify_GameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"

void UPDAnimNotify_GameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!IsValid(MeshComp)||!IsValid(MeshComp->GetOwner())) return;
	if (!EventTag.IsValid()) return;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		MeshComp->GetOwner(), EventTag, FGameplayEventData());
}