// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "PDVisionComponent.generated.h"

class UAbilitySystemComponent;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Vision")
	float ProximityRadius=50.f;

	float VisionRange=1200.f;
	float VisionAngle=90.f;
	float UpdateInterval=0.1f;
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

	void UpdateFogOfWarMPC();

	float StaminaScale=1.f;

	
	
private:
	TSet<AActor*> VisibleActors;
	FTimerHandle TimerHandle;

	bool bHasAngleOverride=false;
	float VisionAngleOverride=0.f;

	FVector LastLocation=FVector::ZeroVector;
	float LastYaw=0.f;
	//Threshold
	float LocationThreshold=5.f;
	float YawThreshold=5.f;
};
