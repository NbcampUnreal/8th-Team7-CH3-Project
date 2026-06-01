#include "Items/Station/PDStorageDoorActor.h"

#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Curves/CurveFloat.h"
#include "Items/Station/PDStashActor.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

APDStorageDoorActor::APDStorageDoorActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void APDStorageDoorActor::BeginPlay()
{
	Super::BeginPlay();

	CacheDoorComponents(true);
	BindToStash();
	CloseDoor(true);
}

void APDStorageDoorActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	CacheDoorComponents(true);
	CurrentAlpha = 0.f;
	StartAlpha = 0.f;
	TargetAlpha = 0.f;
	MovementElapsed = 0.f;
	ApplyDoorAlpha(CurrentAlpha);
}

void APDStorageDoorActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearQueuedActions();
	StopDoorSound();
	UnbindFromStash();
	Super::EndPlay(EndPlayReason);
}

void APDStorageDoorActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	MovementElapsed += DeltaSeconds;
	const float RawAlpha = ActiveMovementDuration > 0.f ? FMath::Clamp(MovementElapsed / ActiveMovementDuration, 0.f, 1.f) : 1.f;
	const float CurveAlpha = MovementCurve ? FMath::Clamp(MovementCurve->GetFloatValue(RawAlpha), 0.f, 1.f) : RawAlpha;

	CurrentAlpha = FMath::Lerp(StartAlpha, TargetAlpha, CurveAlpha);
	ApplyDoorAlpha(CurrentAlpha);

	if (RawAlpha >= 1.f)
	{
		FinishDoorMovement();
	}
}

void APDStorageDoorActor::OpenDoor(bool bInstant)
{
	SetDoorOpen(true, bInstant);
}

void APDStorageDoorActor::CloseDoor(bool bInstant)
{
	SetDoorOpen(false, bInstant);
}

void APDStorageDoorActor::BindToStash()
{
	if (!TargetStash)
	{
		return;
	}

	TargetStash->OnStorageOpened.RemoveAll(this);
	TargetStash->OnStorageClosed.RemoveAll(this);
	TargetStash->OnStorageOpened.AddDynamic(this, &APDStorageDoorActor::HandleStorageOpened);
	TargetStash->OnStorageClosed.AddDynamic(this, &APDStorageDoorActor::HandleStorageClosed);
}

void APDStorageDoorActor::UnbindFromStash()
{
	if (!TargetStash)
	{
		return;
	}

	TargetStash->OnStorageOpened.RemoveAll(this);
	TargetStash->OnStorageClosed.RemoveAll(this);
}

void APDStorageDoorActor::SetDoorOpen(bool bOpen, bool bInstant)
{
	CacheDoorComponents(false);
	ClearQueuedActions();

	const EPDStorageDoorCuePhase RequestedPhase = bOpen ? EPDStorageDoorCuePhase::OpenRequested : EPDStorageDoorCuePhase::CloseRequested;
	PlayNiagaraCues(RequestedPhase);

	if (!bInstant)
	{
		PlayDoorSound(bOpen);
	}

	const float Delay = bOpen ? OpenStartDelay : CloseStartDelay;
	if (bInstant || Delay <= 0.f)
	{
		StartDoorMovement(bOpen, bInstant);
		return;
	}

	bQueuedDelayedMovementOpen = bOpen;
	GetWorldTimerManager().SetTimer(DoorDelayTimerHandle, this, &APDStorageDoorActor::StartDelayedDoorMovement, Delay, false);
}

void APDStorageDoorActor::StartDelayedDoorMovement()
{
	StartDoorMovement(bQueuedDelayedMovementOpen, false);
}

void APDStorageDoorActor::StartDoorMovement(bool bOpen, bool bInstant)
{
	CacheDoorComponents(false);

	const float NewTargetAlpha = bOpen ? 1.f : 0.f;
	TargetAlpha = NewTargetAlpha;

	if (bInstant)
	{
		CurrentAlpha = TargetAlpha;
		StartAlpha = TargetAlpha;
		MovementElapsed = 0.f;
		ApplyDoorAlpha(CurrentAlpha);
		SetActorTickEnabled(false);
		return;
	}

	bMovingOpen = bOpen;
	StartAlpha = CurrentAlpha;
	MovementElapsed = 0.f;
	ActiveMovementDuration = FMath::Max(bOpen ? OpenDuration : CloseDuration, KINDA_SMALL_NUMBER);

	PlayNiagaraCues(bOpen ? EPDStorageDoorCuePhase::OpenMovementStarted : EPDStorageDoorCuePhase::CloseMovementStarted);

	if (FMath::IsNearlyEqual(StartAlpha, TargetAlpha, 0.001f))
	{
		FinishDoorMovement();
		return;
	}

	SetActorTickEnabled(true);
}

void APDStorageDoorActor::FinishDoorMovement()
{
	CurrentAlpha = TargetAlpha;
	ApplyDoorAlpha(CurrentAlpha);
	SetActorTickEnabled(false);
	PlayNiagaraCues(bMovingOpen ? EPDStorageDoorCuePhase::OpenFinished : EPDStorageDoorCuePhase::CloseFinished);
}

void APDStorageDoorActor::ApplyDoorAlpha(float Alpha)
{
	CacheDoorComponents(false);

	if (CachedOverDoorComponent.IsValid())
	{
		CachedOverDoorComponent->SetRelativeLocation(FMath::Lerp(OverDoorClosedLocation, OverDoorClosedLocation + OverDoorOpenOffset, Alpha));
	}

	if (CachedUnderDoorComponent.IsValid())
	{
		CachedUnderDoorComponent->SetRelativeLocation(FMath::Lerp(UnderDoorClosedLocation, UnderDoorClosedLocation + UnderDoorOpenOffset, Alpha));
	}
}

void APDStorageDoorActor::CacheDoorComponents(bool bResetClosedLocations)
{
	if (!CachedOverDoorComponent.IsValid())
	{
		CachedOverDoorComponent = FindDoorComponent(OverDoorComponentName);
	}

	if (!CachedUnderDoorComponent.IsValid())
	{
		CachedUnderDoorComponent = FindDoorComponent(UnderDoorComponentName);
	}

	if (bResetClosedLocations)
	{
		if (CachedOverDoorComponent.IsValid())
		{
			OverDoorClosedLocation = CachedOverDoorComponent->GetRelativeLocation();
		}

		if (CachedUnderDoorComponent.IsValid())
		{
			UnderDoorClosedLocation = CachedUnderDoorComponent->GetRelativeLocation();
		}
	}
}

USceneComponent* APDStorageDoorActor::FindDoorComponent(FName ComponentName) const
{
	if (ComponentName.IsNone())
	{
		return nullptr;
	}

	TArray<USceneComponent*> SceneComponents;
	GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent && SceneComponent->GetFName() == ComponentName)
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

void APDStorageDoorActor::PlayDoorSound(bool bOpen)
{
	StopDoorSound();

	USoundBase* Sound = bOpen ? OpenSound.Get() : CloseSound.Get();
	if (!Sound)
	{
		return;
	}

	UAudioComponent* AudioComponent = UGameplayStatics::SpawnSoundAtLocation(
		this,
		Sound,
		GetActorLocation(),
		FRotator::ZeroRotator,
		SoundVolumeMultiplier,
		SoundPitchMultiplier,
		0.f,
		nullptr,
		nullptr,
		true);

	ActiveDoorSoundComponent = AudioComponent;
}

void APDStorageDoorActor::StopDoorSound()
{
	if (ActiveDoorSoundComponent.IsValid())
	{
		ActiveDoorSoundComponent->Stop();
		ActiveDoorSoundComponent.Reset();
	}
}

void APDStorageDoorActor::ClearQueuedActions()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DoorDelayTimerHandle);
		for (FTimerHandle& TimerHandle : NiagaraTimerHandles)
		{
			World->GetTimerManager().ClearTimer(TimerHandle);
		}
	}

	NiagaraTimerHandles.Reset();
}

void APDStorageDoorActor::PlayNiagaraCues(EPDStorageDoorCuePhase Phase)
{
	for (const FPDStorageDoorNiagaraCue& Cue : NiagaraCues)
	{
		if (Cue.Phase != Phase || !Cue.NiagaraSystem)
		{
			continue;
		}

		if (Cue.Delay <= 0.f)
		{
			SpawnNiagaraCue(Cue);
			continue;
		}

		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate = FTimerDelegate::CreateWeakLambda(this, [this, Cue]()
		{
			SpawnNiagaraCue(Cue);
		});
		GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, Cue.Delay, false);
		NiagaraTimerHandles.Add(TimerHandle);
	}
}

void APDStorageDoorActor::SpawnNiagaraCue(const FPDStorageDoorNiagaraCue& Cue)
{
	if (!Cue.NiagaraSystem)
	{
		return;
	}

	USceneComponent* SpawnPoint = GetCueSpawnPoint(Cue);
	const FTransform BaseTransform = SpawnPoint ? SpawnPoint->GetComponentTransform() : GetActorTransform();
	const FTransform SpawnTransform = Cue.RelativeTransform * BaseTransform;

	if (Cue.bAttachToSpawnPoint && SpawnPoint)
	{
		UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			Cue.NiagaraSystem,
			SpawnPoint,
			NAME_None,
			Cue.RelativeTransform.GetLocation(),
			Cue.RelativeTransform.GetRotation().Rotator(),
			EAttachLocation::KeepRelativeOffset,
			Cue.bAutoDestroy);

		if (NiagaraComponent)
		{
			NiagaraComponent->SetRelativeScale3D(Cue.RelativeTransform.GetScale3D());
		}

		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this,
		Cue.NiagaraSystem,
		SpawnTransform.GetLocation(),
		SpawnTransform.GetRotation().Rotator(),
		SpawnTransform.GetScale3D(),
		Cue.bAutoDestroy);
}

USceneComponent* APDStorageDoorActor::GetCueSpawnPoint(const FPDStorageDoorNiagaraCue& Cue) const
{
	if (Cue.SpawnPointComponentName.IsNone())
	{
		return nullptr;
	}

	return FindDoorComponent(Cue.SpawnPointComponentName);
}

void APDStorageDoorActor::HandleStorageOpened(APDStashActor* StashActor)
{
	if (StashActor == TargetStash)
	{
		OpenDoor(false);
	}
}

void APDStorageDoorActor::HandleStorageClosed(APDStashActor* StashActor)
{
	if (StashActor == TargetStash)
	{
		CloseDoor(false);
	}
}
