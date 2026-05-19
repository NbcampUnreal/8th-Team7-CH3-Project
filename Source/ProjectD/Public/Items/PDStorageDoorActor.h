#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDStorageDoorActor.generated.h"

class APDStashActor;
class UCurveFloat;
class UNiagaraSystem;
class UAudioComponent;
class USceneComponent;
class USoundBase;


UENUM(BlueprintType)
enum class EPDStorageDoorCuePhase : uint8
{
	OpenRequested,
	OpenMovementStarted,
	OpenFinished,
	CloseRequested,
	CloseMovementStarted,
	CloseFinished
};

USTRUCT(BlueprintType)
struct FPDStorageDoorNiagaraCue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Niagara")
	TObjectPtr<UNiagaraSystem> NiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Niagara")
	EPDStorageDoorCuePhase Phase = EPDStorageDoorCuePhase::OpenMovementStarted;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Niagara", meta = (ClampMin = "0.0"))
	float Delay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Niagara")
	FName SpawnPointComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Niagara")
	FTransform RelativeTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Niagara")
	bool bAttachToSpawnPoint = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Niagara")
	bool bAutoDestroy = true;
};

UCLASS(Blueprintable)
class PROJECTD_API APDStorageDoorActor : public AActor
{
	GENERATED_BODY()

public:
	APDStorageDoorActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "PD|StorageDoor")
	void OpenDoor(bool bInstant = false);

	UFUNCTION(BlueprintCallable, Category = "PD|StorageDoor")
	void CloseDoor(bool bInstant = false);

	UFUNCTION(BlueprintCallable, Category = "PD|StorageDoor")
	void BindToStash();

	UFUNCTION(BlueprintCallable, Category = "PD|StorageDoor")
	void UnbindFromStash();

	UFUNCTION(BlueprintCallable, Category = "PD|StorageDoor|Niagara")
	void PlayNiagaraCues(EPDStorageDoorCuePhase Phase);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "PD|StorageDoor")
	TObjectPtr<APDStashActor> TargetStash;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Components")
	FName OverDoorComponentName = TEXT("SM_OverDoor");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Components")
	FName UnderDoorComponentName = TEXT("SM_UnderDoor");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Movement")
	FVector OverDoorOpenOffset = FVector(0.f, 0.f, 300.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Movement")
	FVector UnderDoorOpenOffset = FVector(0.f, 0.f, -300.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Movement", meta = (ClampMin = "0.0"))
	float OpenStartDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Movement", meta = (ClampMin = "0.01"))
	float OpenDuration = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Movement", meta = (ClampMin = "0.0"))
	float CloseStartDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Movement", meta = (ClampMin = "0.01"))
	float CloseDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Movement")
	TObjectPtr<UCurveFloat> MovementCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Sound")
	TObjectPtr<USoundBase> OpenSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Sound")
	TObjectPtr<USoundBase> CloseSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Sound", meta = (ClampMin = "0.0"))
	float SoundVolumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Sound", meta = (ClampMin = "0.0"))
	float SoundPitchMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|StorageDoor|Niagara")
	TArray<FPDStorageDoorNiagaraCue> NiagaraCues;

private:
	void SetDoorOpen(bool bOpen, bool bInstant);
	void StartDoorMovement(bool bOpen, bool bInstant);
	void StartDelayedDoorMovement();
	void FinishDoorMovement();
	void ApplyDoorAlpha(float Alpha);
	void CacheDoorComponents(bool bResetClosedLocations);
	USceneComponent* FindDoorComponent(FName ComponentName) const;
	void PlayDoorSound(bool bOpen);
	void StopDoorSound();
	void ClearQueuedActions();
	void SpawnNiagaraCue(const FPDStorageDoorNiagaraCue& Cue);
	USceneComponent* GetCueSpawnPoint(const FPDStorageDoorNiagaraCue& Cue) const;

	UFUNCTION()
	void HandleStorageOpened(APDStashActor* StashActor);

	UFUNCTION()
	void HandleStorageClosed(APDStashActor* StashActor);

	float CurrentAlpha = 0.f;
	float StartAlpha = 0.f;
	float TargetAlpha = 0.f;
	float MovementElapsed = 0.f;
	float ActiveMovementDuration = 1.f;
	bool bMovingOpen = false;
	bool bQueuedDelayedMovementOpen = false;

	FTimerHandle DoorDelayTimerHandle;
	TArray<FTimerHandle> NiagaraTimerHandles;
	TWeakObjectPtr<UAudioComponent> ActiveDoorSoundComponent;

	TWeakObjectPtr<USceneComponent> CachedOverDoorComponent;
	TWeakObjectPtr<USceneComponent> CachedUnderDoorComponent;
	FVector OverDoorClosedLocation = FVector::ZeroVector;
	FVector UnderDoorClosedLocation = FVector::ZeroVector;
};
