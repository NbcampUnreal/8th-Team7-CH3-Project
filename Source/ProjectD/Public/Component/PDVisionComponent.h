

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "PDVisionComponent.generated.h"

class UAbilitySystemComponent;
class UMaterialParameterCollection;

namespace EEndPlayReason
{
	enum Type : int;
}

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDVisionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDVisionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="PD|Vision")
	void BindToAttributeSet(UAbilitySystemComponent* ASC);

	UFUNCTION(BlueprintCallable, Category="PD|Vision")
	void SetVisionAngleOverride(float Angle);

	UFUNCTION(BlueprintCallable, Category="PD|Vision")
	void ClearVisionAngleOverride();

	UFUNCTION(BlueprintCallable, Category="PD|Vision")
	const TSet<AActor*>& GetVisibleActors() const { return VisibleActors; }

	UFUNCTION(BlueprintCallable, Category="PD|Vision")
	void UpdateStaminaScale(float Scale);

	UPROPERTY(EditDefaultsOnly, Category="PD|Vision")
	TObjectPtr<UMaterialParameterCollection> FogOfWarMPC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Vision", meta=(ClampMin="1", ClampMax="4"))
	int32 MaxSharedVisionSources = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Vision")
	float ProximityRadius=50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Vision", meta=(DisplayName="Vision Radius", ClampMin="0.0"))
	float VisionRange=1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Vision", meta=(DisplayName="Vision Angle", ClampMin="0.0", ClampMax="360.0"))
	float VisionAngle=90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Vision", meta=(ClampMin="0.01"))
	float UpdateInterval=0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Vision", meta=(ClampMin="0.01"))
	float ThrottledInterval=0.5f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;

private:
	void PerformVisionUpdate();
	bool IsInCone(AActor* Target) const;
	bool HasLineOfSight(AActor* Target) const;
	void UpdateExposure(AActor* Target, float Exposure);
	void ScheduleNextUpdate(float Interval);
	float CalcExposure(AActor* Target) const;
	float GetEffectiveAngle() const;

	void OnVisionRangeChanged(const FOnAttributeChangeData& Data);
	void OnVisionAngleChanged(const FOnAttributeChangeData& Data);
	void OnVisionUpdateIntervalChanged(const FOnAttributeChangeData& Data);

	void UpdateFogOfWarMPC_Transform(float DeltaTime);
	void UpdateFogOfWarMPC_Vision();
	void UpdateSharedFogOfWarMPC(float DeltaTime);
	void WriteVisionSourceToMPC(int32 SourceIndex, const FVector& Location, const FVector& Forward, float Range, float AngleCos, float InProximityRadius) const;

	float StaminaScale=1.f;

private:
	TSet<AActor*> VisibleActors;
	FTimerHandle TimerHandle;

	bool bHasAngleOverride=false;
	float VisionAngleOverride=0.f;

	FVector LastLocation=FVector::ZeroVector;
	float LastYaw=0.f;

	float LocationThreshold=5.f;
	float YawThreshold=5.f;

	float ForwardSmoothSpeed=10.f;
	FVector SmoothedForward=FVector::ForwardVector;
};
