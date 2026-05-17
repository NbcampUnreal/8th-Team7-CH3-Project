#include "Animation/PDFootstepDataAsset.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

const FPDFootstepEntry& UPDFootstepDataAsset::GetEntryForSurface(EPhysicalSurface Surface) const
{
	if (const FPDFootstepEntry* Found = SurfaceEntries.Find(Surface))
	{
		return *Found;
	}
	return DefaultEntry;
}

void UPDFootstepDataAsset::PlayFootstep(
	USkeletalMeshComponent* MeshComp,
	const UPDFootstepDataAsset* Data,
	FName FootSocket,
	float TraceDistance)
{
	if (!MeshComp || !Data) return;
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	const FVector FootLocation = MeshComp->DoesSocketExist(FootSocket)
		? MeshComp->GetSocketLocation(FootSocket)
		: Owner->GetActorLocation();

	// 표면 감지 — PhysMaterial 없으면 Default 엔트리로 fallback.
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

	const FPDFootstepEntry& Entry = Data->GetEntryForSurface(Surface);
	if (!Entry.Sound) return;

	UGameplayStatics::PlaySoundAtLocation(MeshComp, Entry.Sound, FootLocation);

	if (Entry.StepVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(MeshComp, Entry.StepVFX, FootLocation);
	}

	// AI 청각용 노이즈 — Pawn 이면 Instigator 명시.
	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		Pawn->MakeNoise(1.0f, Pawn, FootLocation, Entry.NoiseRange);
	}
	else
	{
		Owner->MakeNoise(1.0f, nullptr, FootLocation, Entry.NoiseRange);
	}
}
