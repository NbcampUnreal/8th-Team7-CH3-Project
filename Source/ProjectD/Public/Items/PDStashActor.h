#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/PDInteractable.h"
#include "PDStashActor.generated.h"

class APDPlayerController;
class UBoxComponent;
class UPDStashComponent;
class USoundBase;

UCLASS(Blueprintable)
class PROJECTD_API APDStashActor : public AActor, public IPDInteractable
{
	GENERATED_BODY()

public:
	APDStashActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	FORCEINLINE UPDStashComponent* GetStashComponent() const { return StashComponent; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Stash")
	TObjectPtr<UBoxComponent> InteractionCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Stash")
	TObjectPtr<UPDStashComponent> StashComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Door")
	FName DoorBoneName = TEXT("Door_Hinge_01");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Door")
	FVector DoorRotationAxis = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Door")
	float ClosedDoorAngle = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Door")
	float OpenDoorAngle = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Door", meta = (ClampMin = "0.0"))
	float DoorInterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Sound")
	TObjectPtr<USoundBase> OpenSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Sound")
	TObjectPtr<USoundBase> CloseSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Sound", meta = (ClampMin = "0.0"))
	float SoundVolumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Sound", meta = (ClampMin = "0.0"))
	float SoundPitchMultiplier = 1.f;

private:
	void ConfigureInteractionCollision() const;
	void SetDoorOpen(bool bOpen, bool bInstant = false);
	void ApplyDoorAngle(float Angle);
	void PlayDoorSound(bool bOpen) const;
	void BindStashClose(APDPlayerController* PlayerController);
	void UnbindStashClose();
	void HandleStashInterfaceClosed(UPDStashComponent* ClosedStashComponent);

	float CurrentDoorAngle = 100.f;
	float TargetDoorAngle = 100.f;

	TWeakObjectPtr<APDPlayerController> BoundPlayerController;
};
