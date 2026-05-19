#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDEquipmentModificationRollingActor.generated.h"

class APDEquipmentModificationActor;
class UAudioComponent;
class USceneComponent;
class USoundBase;

UCLASS(Blueprintable)
class PROJECTD_API APDEquipmentModificationRollingActor : public AActor
{
	GENERATED_BODY()

public:
	APDEquipmentModificationRollingActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification|Rolling")
	void StartRolling(bool bInstant = false);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification|Rolling")
	void StopRolling(bool bInstant = false);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification|Rolling")
	void BindToEquipmentModificationActor();

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification|Rolling")
	void UnbindFromEquipmentModificationActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Rolling")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Rolling")
	TObjectPtr<APDEquipmentModificationActor> TargetEquipmentModificationActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Rolling")
	FVector LocalRotationAxis = FVector(0.f, 0.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Rolling", meta = (ClampMin = "0.0"))
	float MaxRotationSpeedDegreesPerSecond = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Rolling", meta = (ClampMin = "0.0"))
	float StartDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Rolling", meta = (ClampMin = "0.0"))
	float StopDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Rolling", meta = (ClampMin = "0.01"))
	float AccelerationDuration = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Rolling", meta = (ClampMin = "0.01"))
	float DecelerationDuration = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Rolling|Sound")
	TObjectPtr<USoundBase> StartSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Rolling|Sound")
	TObjectPtr<USoundBase> StopSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Rolling|Sound", meta = (ClampMin = "0.0"))
	float SoundVolumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Rolling|Sound", meta = (ClampMin = "0.0"))
	float SoundPitchMultiplier = 1.f;

private:
	void BeginSpeedTransition(float NewTargetSpeed, float Duration, bool bInstant);
	void StartDelayedRolling();
	void StopDelayedRolling();
	void ClearRollingDelay();
	void PlayRollingSound(bool bStarting);
	void StopRollingSound();

	UFUNCTION()
	void HandleEquipmentModificationOpened(APDEquipmentModificationActor* ModificationActor);

	UFUNCTION()
	void HandleEquipmentModificationClosed(APDEquipmentModificationActor* ModificationActor);

	float CurrentRotationSpeed = 0.f;
	float StartRotationSpeed = 0.f;
	float TargetRotationSpeed = 0.f;
	float TransitionElapsed = 0.f;
	float ActiveTransitionDuration = 1.f;

	FTimerHandle RollingDelayTimerHandle;
	TWeakObjectPtr<UAudioComponent> ActiveRollingSoundComponent;
};
