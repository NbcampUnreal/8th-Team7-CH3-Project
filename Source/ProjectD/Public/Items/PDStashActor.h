#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/PDInteractable.h"
#include "PDStashActor.generated.h"

class APDStashActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDStashStateChangedSignature, APDStashActor*, StashActor);

class APDPlayerController;
class UBoxComponent;
class UPDInteractionOutlineComponent;
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Stash")
	TObjectPtr<UPDInteractionOutlineComponent> OutlineComponent;

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

protected:
	// LootBox 등 파생 클래스가 자체 close 델리게이트(OnLootInterfaceClosed 등)에 바인딩하기 위해 protected.
	void SetDoorOpen(bool bOpen, bool bInstant = false);

	/** 컨테이너 close 델리게이트 구독. 기본 구현은 OnStashInterfaceClosed. 자식은 다른 델리게이트로 override. */
	virtual void BindContainerClose(APDPlayerController* PlayerController);
	virtual void UnbindContainerClose();

	/** close 시그널 공통 처리 — 자기 컴포넌트와 매칭되면 문 닫고 unbind. 시그니처는 양쪽 델리게이트가 동일. */
	UFUNCTION()
	void HandleContainerClosed(UPDStashComponent* ClosedStashComponent);

	TWeakObjectPtr<APDPlayerController> BoundPlayerController;

private:
	void ConfigureInteractionCollision() const;
	void ApplyDoorAngle(float Angle);
	void PlayDoorSound(bool bOpen) const;

	float CurrentDoorAngle = 100.f;
	float TargetDoorAngle = 100.f;
};
