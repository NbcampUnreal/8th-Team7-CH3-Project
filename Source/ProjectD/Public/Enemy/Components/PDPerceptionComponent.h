#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionComponent.h"
#include "PDPerceptionComponent.generated.h"

class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnTargetSensed, AActor*, Actor, bool, bSensed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (FPDOnTargetSpotted, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnTargetLost, AActor*, Target, FVector, LastKnownLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnNoiseHeard, AActor*, NoiseInstigator, FVector, Location);

/**
 * UAIPerceptionComponent 래퍼. Sight + Hearing 자극을 도메인 이벤트로 변환.
 * AIController가 자동으로 PerceptionComponent 슬롯을 인식하므로 별도 통합 필요 없음.
 */
UCLASS(ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDPerceptionComponent : public UAIPerceptionComponent
{
	GENERATED_BODY()

public:
	UPDPerceptionComponent();

	virtual void BeginPlay() override;

	UPROPERTY(BlueprintAssignable, Category = "PD|Perception")
	FPDOnTargetSensed OnTargetSensed;

	UPROPERTY(BlueprintAssignable, Category = "PD|Perception")
	FPDOnTargetSpotted OnTargetSpotted;

	UPROPERTY(BlueprintAssignable, Category = "PD|Perception")
	FPDOnTargetLost OnTargetLost;

	UPROPERTY(BlueprintAssignable, Category = "PD|Perception")
	FPDOnNoiseHeard OnNoiseHeard;

	UFUNCTION(BlueprintPure, Category = "PD|Perception") FORCEINLINE float GetSightRadius()                 const { return SightRadius; }
	UFUNCTION(BlueprintPure, Category = "PD|Perception") FORCEINLINE float GetLoseSightRadius()             const { return LoseSightRadius; }
	UFUNCTION(BlueprintPure, Category = "PD|Perception") FORCEINLINE float GetPeripheralVisionAngleDegrees() const { return PeripheralVisionAngleDegrees; }
	UFUNCTION(BlueprintPure, Category = "PD|Perception") FORCEINLINE float GetHearingRange()                const { return HearingRange; }
	
	// SightRadius setter
	UFUNCTION(BlueprintCallable, Category = "PD|Perception") void SetSightRadius(float NewSightRadius) { SightRadius = NewSightRadius; };


protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Perception|Sight", meta = (ClampMin = "0.0"))
	float SightRadius = 800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Perception|Sight", meta = (ClampMin = "0.0"))
	float LoseSightRadius = SightRadius + 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Perception|Sight", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float PeripheralVisionAngleDegrees = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Perception|Sight", meta = (ClampMin = "0.0"))
	float SightMaxAge = 4.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Perception|Hearing", meta = (ClampMin = "0.0"))
	float HearingRange = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Perception|Hearing", meta = (ClampMin = "0.0"))
	float HearingMaxAge = 3.f;

private:
	UFUNCTION()
	void HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;
};
