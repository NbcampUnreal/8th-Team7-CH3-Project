#include "Animation/PDFootStepNotify.h"
#include "Animation/PDFootstepDataAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"

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

    if (!MeshComp || !FootstepData) return;
    AActor* Owner = MeshComp->GetOwner();
    if (!Owner) return;

    //발 위치
    const FName Socket = (Foot == EPDFoot::Left) ? TEXT("foot_l") : TEXT("foot_r");
    const FVector FootLocation = MeshComp->DoesSocketExist(Socket)
        ? MeshComp->GetSocketLocation(Socket)
        : Owner->GetActorLocation();

    //Surface 감지 (LineTrace)
    EPhysicalSurface Surface = SurfaceType_Default;
    if (UWorld* World = MeshComp->GetWorld())
    {
        FCollisionQueryParams Params(SCENE_QUERY_STAT(PDFootstep), true);
        Params.bReturnPhysicalMaterial = true;
        Params.AddIgnoredActor(Owner);

        FHitResult Hit;
        const FVector End = FootLocation - FVector(0, 0, TraceDistance);
        if (World->LineTraceSingleByChannel(Hit, FootLocation, End, ECC_Visibility, Params))
        {
            if (Hit.PhysMaterial.IsValid())
            {
                Surface = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
            }
        }
    }

    //DataAsset에서 항목 조회
    const FPDFootstepEntry& Entry = FootstepData->GetEntryForSurface(Surface);
    if (!Entry.Sound) return;

    //사운드 재생
    UGameplayStatics::PlaySoundAtLocation(MeshComp, Entry.Sound, FootLocation);

    //VFX
    if (Entry.StepVFX)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(MeshComp, Entry.StepVFX, FootLocation);
    }

    //노이즈
    if (APawn* Pawn = Cast<APawn>(Owner))
    {
        Pawn->MakeNoise(1.0f, Pawn, FootLocation, Entry.NoiseRange);
    }
    else
    {
        Owner->MakeNoise(1.0f, nullptr, FootLocation, Entry.NoiseRange);
    }
}