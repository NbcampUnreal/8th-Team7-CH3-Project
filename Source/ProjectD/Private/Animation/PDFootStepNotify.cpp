#include "Animation/PDFootStepNotify.h"
#include "Animation/PDFootstepDataAsset.h"
#include "Components/SkeletalMeshComponent.h"

UPDFootStepNotify::UPDFootStepNotify()
{
#if WITH_EDITORONLY_DATA
    NotifyColor = FColor(150, 200, 255);
#endif
}

FString UPDFootStepNotify::GetNotifyName_Implementation() const
{
    return (Foot == EPDFoot::Left) ? TEXT("Footstep_L") : TEXT("Footstep_R");
}

void UPDFootStepNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    const FName Socket = (Foot == EPDFoot::Left) ? TEXT("foot_l") : TEXT("foot_r");
    UPDFootstepDataAsset::PlayFootstep(MeshComp, FootstepData, Socket, TraceDistance);
}
