#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDEquipmentModificationLiftActor.generated.h"

class APDEquipmentModificationActor;
class UCurveFloat;
class USceneComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDEquipmentModificationLiftActor : public AActor
{
	GENERATED_BODY()

public:
	APDEquipmentModificationLiftActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification|Lift")
	void RaiseActor(bool bInstant = false);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification|Lift")
	void LowerActor(bool bInstant = false);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification|Lift")
	void BindToEquipmentModificationActor();

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification|Lift")
	void UnbindFromEquipmentModificationActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Lift")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Lift")
	TObjectPtr<APDEquipmentModificationActor> TargetEquipmentModificationActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Lift")
	FVector RaisedWorldOffset = FVector(0.f, 0.f, 200.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Lift", meta = (ClampMin = "0.0"))
	float RaiseStartDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Lift", meta = (ClampMin = "0.0"))
	float LowerStartDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Lift", meta = (ClampMin = "0.01"))
	float RaiseDuration = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Lift", meta = (ClampMin = "0.01"))
	float LowerDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Lift")
	TObjectPtr<UCurveFloat> MovementCurve = nullptr;

private:
	void SetRaised(bool bRaised, bool bInstant);
	void StartLiftMovement(bool bRaised, bool bInstant);
	void StartDelayedLiftMovement();
	void FinishLiftMovement();
	void ApplyLiftAlpha(float Alpha);
	void ClearLiftDelay();

	UFUNCTION()
	void HandleEquipmentModificationOpened(APDEquipmentModificationActor* ModificationActor);

	UFUNCTION()
	void HandleEquipmentModificationClosed(APDEquipmentModificationActor* ModificationActor);

	FVector ClosedWorldLocation = FVector::ZeroVector;
	float CurrentAlpha = 0.f;
	float StartAlpha = 0.f;
	float TargetAlpha = 0.f;
	float MovementElapsed = 0.f;
	float ActiveMovementDuration = 1.f;
	bool bQueuedRaised = false;

	FTimerHandle LiftDelayTimerHandle;
};
