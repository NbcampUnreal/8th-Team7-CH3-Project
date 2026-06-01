#include "Items/Station/PDEquipmentModificationRollingActor.h"

#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Items/Station/PDEquipmentModificationActor.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

APDEquipmentModificationRollingActor::APDEquipmentModificationRollingActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void APDEquipmentModificationRollingActor::BeginPlay()
{
	Super::BeginPlay();

	BindToEquipmentModificationActor();
	StopRolling(true);
}

void APDEquipmentModificationRollingActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearRollingDelay();
	StopRollingSound();
	UnbindFromEquipmentModificationActor();
	Super::EndPlay(EndPlayReason);
}

void APDEquipmentModificationRollingActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TransitionElapsed += DeltaSeconds;
	const float Alpha = ActiveTransitionDuration > 0.f ? FMath::Clamp(TransitionElapsed / ActiveTransitionDuration, 0.f, 1.f) : 1.f;
	CurrentRotationSpeed = FMath::Lerp(StartRotationSpeed, TargetRotationSpeed, Alpha);

	const FVector Axis = LocalRotationAxis.GetSafeNormal();
	if (!Axis.IsNearlyZero() && !FMath::IsNearlyZero(CurrentRotationSpeed))
	{
		const FQuat DeltaRotation(Axis, FMath::DegreesToRadians(CurrentRotationSpeed * DeltaSeconds));
		AddActorLocalRotation(DeltaRotation);
	}

	if (Alpha >= 1.f && FMath::IsNearlyZero(TargetRotationSpeed, 0.01f))
	{
		CurrentRotationSpeed = 0.f;
		SetActorTickEnabled(false);
	}
}

void APDEquipmentModificationRollingActor::StartRolling(bool bInstant)
{
	ClearRollingDelay();
	if (!bInstant)
	{
		PlayRollingSound(true);
	}

	if (bInstant || StartDelay <= 0.f)
	{
		BeginSpeedTransition(MaxRotationSpeedDegreesPerSecond, AccelerationDuration, bInstant);
		return;
	}

	GetWorldTimerManager().SetTimer(RollingDelayTimerHandle, this, &APDEquipmentModificationRollingActor::StartDelayedRolling, StartDelay, false);
}

void APDEquipmentModificationRollingActor::StopRolling(bool bInstant)
{
	ClearRollingDelay();
	if (!bInstant)
	{
		PlayRollingSound(false);
	}

	if (bInstant || StopDelay <= 0.f)
	{
		BeginSpeedTransition(0.f, DecelerationDuration, bInstant);
		return;
	}

	GetWorldTimerManager().SetTimer(RollingDelayTimerHandle, this, &APDEquipmentModificationRollingActor::StopDelayedRolling, StopDelay, false);
}

void APDEquipmentModificationRollingActor::BindToEquipmentModificationActor()
{
	if (!TargetEquipmentModificationActor)
	{
		return;
	}

	TargetEquipmentModificationActor->OnEquipmentModificationOpened.RemoveAll(this);
	TargetEquipmentModificationActor->OnEquipmentModificationClosed.RemoveAll(this);
	TargetEquipmentModificationActor->OnEquipmentModificationOpened.AddDynamic(this, &APDEquipmentModificationRollingActor::HandleEquipmentModificationOpened);
	TargetEquipmentModificationActor->OnEquipmentModificationClosed.AddDynamic(this, &APDEquipmentModificationRollingActor::HandleEquipmentModificationClosed);
}

void APDEquipmentModificationRollingActor::UnbindFromEquipmentModificationActor()
{
	if (!TargetEquipmentModificationActor)
	{
		return;
	}

	TargetEquipmentModificationActor->OnEquipmentModificationOpened.RemoveAll(this);
	TargetEquipmentModificationActor->OnEquipmentModificationClosed.RemoveAll(this);
}

void APDEquipmentModificationRollingActor::BeginSpeedTransition(float NewTargetSpeed, float Duration, bool bInstant)
{
	TargetRotationSpeed = NewTargetSpeed;

	if (bInstant)
	{
		CurrentRotationSpeed = TargetRotationSpeed;
		StartRotationSpeed = TargetRotationSpeed;
		TransitionElapsed = 0.f;
		SetActorTickEnabled(!FMath::IsNearlyZero(TargetRotationSpeed, 0.01f));
		return;
	}

	StartRotationSpeed = CurrentRotationSpeed;
	TransitionElapsed = 0.f;
	ActiveTransitionDuration = FMath::Max(Duration, KINDA_SMALL_NUMBER);
	SetActorTickEnabled(true);
}

void APDEquipmentModificationRollingActor::StartDelayedRolling()
{
	BeginSpeedTransition(MaxRotationSpeedDegreesPerSecond, AccelerationDuration, false);
}

void APDEquipmentModificationRollingActor::StopDelayedRolling()
{
	BeginSpeedTransition(0.f, DecelerationDuration, false);
}

void APDEquipmentModificationRollingActor::ClearRollingDelay()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RollingDelayTimerHandle);
	}
}

void APDEquipmentModificationRollingActor::PlayRollingSound(bool bStarting)
{
	StopRollingSound();

	USoundBase* Sound = bStarting ? StartSound.Get() : StopSound.Get();
	if (!Sound)
	{
		return;
	}

	UAudioComponent* AudioComponent = UGameplayStatics::SpawnSoundAtLocation(
		this,
		Sound,
		GetActorLocation(),
		GetActorRotation(),
		SoundVolumeMultiplier,
		SoundPitchMultiplier,
		0.f,
		nullptr,
		nullptr,
		true);

	ActiveRollingSoundComponent = AudioComponent;
}

void APDEquipmentModificationRollingActor::StopRollingSound()
{
	if (ActiveRollingSoundComponent.IsValid())
	{
		ActiveRollingSoundComponent->Stop();
		ActiveRollingSoundComponent.Reset();
	}
}

void APDEquipmentModificationRollingActor::HandleEquipmentModificationOpened(APDEquipmentModificationActor* ModificationActor)
{
	if (ModificationActor == TargetEquipmentModificationActor)
	{
		StartRolling(false);
	}
}

void APDEquipmentModificationRollingActor::HandleEquipmentModificationClosed(APDEquipmentModificationActor* ModificationActor)
{
	if (ModificationActor == TargetEquipmentModificationActor)
	{
		StopRolling(false);
	}
}
