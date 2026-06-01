#include "Animation/PDFootStepNotify.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/PDFootstepDataAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameplayTag/PDGameplayTags.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

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

	const FName Socket = (Foot == EPDFoot::Left) ? TEXT("foot_l") : TEXT("foot_r");
	const FVector FootLocation = MeshComp->DoesSocketExist(Socket)
		? MeshComp->GetSocketLocation(Socket)
		: Owner->GetActorLocation();

	EPhysicalSurface Surface = SurfaceType_Default;
	if (UWorld* World = MeshComp->GetWorld())
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(PDFootstep), true);
		Params.bReturnPhysicalMaterial = true;
		Params.AddIgnoredActor(Owner);

		FHitResult Hit;
		const FVector End = FootLocation - FVector(0.f, 0.f, TraceDistance);
		if (World->LineTraceSingleByChannel(Hit, FootLocation, End, ECC_Visibility, Params))
		{
			if (Hit.PhysMaterial.IsValid())
			{
				Surface = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
			}
		}
	}

	const FPDFootstepEntry& Entry = FootstepData->GetEntryForSurface(Surface);
	if (!Entry.Sound && !Entry.StepVFX) return;

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = FootLocation;
		CueParams.TargetAttachComponent = MeshComp;
		CueParams.RawMagnitude = static_cast<float>(Surface);
		CueParams.SourceObject = FootstepData;
		ASC->ExecuteGameplayCue(PDGameplayTags::GameplayCue_Character_Footstep, CueParams);
	}

	if (Owner->HasAuthority())
	{
		if (APawn* Pawn = Cast<APawn>(Owner))
		{
			Pawn->MakeNoise(1.0f, Pawn, FootLocation, Entry.NoiseRange);
		}
		else
		{
			Owner->MakeNoise(1.0f, nullptr, FootLocation, Entry.NoiseRange);
		}
	}
}
