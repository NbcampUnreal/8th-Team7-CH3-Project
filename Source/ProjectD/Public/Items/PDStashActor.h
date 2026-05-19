#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/PDInteractable.h"
#include "PDStashActor.generated.h"

class APDStashActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDStashStateChangedSignature, APDStashActor*, StashActor);

class APDPlayerController;
class AActor;
class UBoxComponent;
class UPDStashComponent;
class USceneComponent;
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

	UPROPERTY(BlueprintAssignable, Category = "PD|Stash|Events")
	FPDStashStateChangedSignature OnStorageOpened;

	UPROPERTY(BlueprintAssignable, Category = "PD|Stash|Events")
	FPDStashStateChangedSignature OnStorageClosed;

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

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "PD|Stash|Linked Door")
	TObjectPtr<AActor> LinkedStorageDoor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Linked Door")
	FName OverDoorComponentName = TEXT("SM_OverDoor");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Linked Door")
	FName UnderDoorComponentName = TEXT("SM_UnderDoor");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Linked Door")
	FVector OverDoorOpenOffset = FVector(0.f, 0.f, 300.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Linked Door")
	FVector UnderDoorOpenOffset = FVector(0.f, 0.f, -300.f);

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
	void CacheLinkedDoorComponents(bool bResetClosedLocations);
	USceneComponent* FindLinkedDoorComponent(FName ComponentName) const;
	float GetDoorOpenAlpha(float Angle) const;
	void PlayDoorSound(bool bOpen) const;
	void BindStashClose(APDPlayerController* PlayerController);
	void UnbindStashClose();
	void HandleStashInterfaceClosed(UPDStashComponent* ClosedStashComponent);

	float CurrentDoorAngle = 100.f;
	float TargetDoorAngle = 100.f;

	TWeakObjectPtr<USceneComponent> CachedOverDoorComponent;
	TWeakObjectPtr<USceneComponent> CachedUnderDoorComponent;
	FVector OverDoorClosedLocation = FVector::ZeroVector;
	FVector UnderDoorClosedLocation = FVector::ZeroVector;

	TWeakObjectPtr<APDPlayerController> BoundPlayerController;
};
