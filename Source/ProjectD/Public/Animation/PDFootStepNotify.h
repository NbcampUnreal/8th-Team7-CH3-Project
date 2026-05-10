#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PDFootStepNotify.generated.h"

class UPDFootstepDataAsset;

UENUM(BlueprintType)
enum class EPDFoot : uint8
{
	Left UMETA(DisplayName="Left Foot"),
	Right UMETA(DisplayName="Right Foot")
};

UCLASS(meta=(DisplayName="PD Footstep Notify"))
class PROJECTD_API UPDFootStepNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	UPDFootStepNotify();

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footstep")
	EPDFoot Foot = EPDFoot::Left;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footstep")
	TObjectPtr<UPDFootstepDataAsset> FootstepData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footstep",
			  meta=(ClampMin="0.0"))
	float TraceDistance = 100.f;
};