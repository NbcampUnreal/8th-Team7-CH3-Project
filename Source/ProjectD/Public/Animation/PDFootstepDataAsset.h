#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PDFootstepDataAsset.generated.h"

class USoundBase;
class UNiagaraSystem;

//한 표면에 대한 발소리/노이즈/VFX 정보
USTRUCT(BlueprintType)
struct FPDFootstepEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footstep")
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footstep")
	TObjectPtr<UNiagaraSystem> StepVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footstep", meta=(ClampMin="0.0"))
	float NoiseRange = 800.f;
};

UCLASS(BlueprintType)
class PROJECTD_API UPDFootstepDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	//Surface별 발소리 매핑
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footstep")
	TMap<TEnumAsByte<EPhysicalSurface>, FPDFootstepEntry> SurfaceEntries;

	//Surface 매핑에 없을 때 fallback
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footstep")
	FPDFootstepEntry DefaultEntry;

	//Surface로 항목 조회. 없으면 DefaultEntry 반환
	UFUNCTION(BlueprintPure, Category="Footstep")
	const FPDFootstepEntry& GetEntryForSurface(EPhysicalSurface Surface) const;
};
