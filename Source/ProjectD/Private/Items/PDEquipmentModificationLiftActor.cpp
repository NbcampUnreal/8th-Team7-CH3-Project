#include "Items/PDEquipmentModificationLiftActor.h"

#include "Components/SceneComponent.h"
#include "Curves/CurveFloat.h"
#include "Items/PDEquipmentModificationActor.h"
#include "TimerManager.h"

APDEquipmentModificationLiftActor::APDEquipmentModificationLiftActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void APDEquipmentModificationLiftActor::BeginPlay()
{
	Super::BeginPlay();

	ClosedWorldLocation = GetActorLocation();
	BindToEquipmentModificationActor();
	LowerActor(true);
}

void APDEquipmentModificationLiftActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		ClosedWorldLocation = GetActorLocation();
		CurrentAlpha = 0.f;
		StartAlpha = 0.f;
		TargetAlpha = 0.f;
	}
}

void APDEquipmentModificationLiftActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearLiftDelay();
	UnbindFromEquipmentModificationActor();
	Super::EndPlay(EndPlayReason);
}

void APDEquipmentModificationLiftActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	MovementElapsed += DeltaSeconds;
	const float RawAlpha = ActiveMovementDuration > 0.f ? FMath::Clamp(MovementElapsed / ActiveMovementDuration, 0.f, 1.f) : 1.f;
	const float CurveAlpha = MovementCurve ? FMath::Clamp(MovementCurve->GetFloatValue(RawAlpha), 0.f, 1.f) : RawAlpha;

	CurrentAlpha = FMath::Lerp(StartAlpha, TargetAlpha, CurveAlpha);
	ApplyLiftAlpha(CurrentAlpha);

	if (RawAlpha >= 1.f)
	{
		FinishLiftMovement();
	}
}

void APDEquipmentModificationLiftActor::RaiseActor(bool bInstant)
{
	SetRaised(true, bInstant);
}

void APDEquipmentModificationLiftActor::LowerActor(bool bInstant)
{
	SetRaised(false, bInstant);
}

void APDEquipmentModificationLiftActor::BindToEquipmentModificationActor()
{
	if (!TargetEquipmentModificationActor)
	{
		return;
	}

	TargetEquipmentModificationActor->OnEquipmentModificationOpened.RemoveAll(this);
	TargetEquipmentModificationActor->OnEquipmentModificationClosed.RemoveAll(this);
	TargetEquipmentModificationActor->OnEquipmentModificationOpened.AddDynamic(this, &APDEquipmentModificationLiftActor::HandleEquipmentModificationOpened);
	TargetEquipmentModificationActor->OnEquipmentModificationClosed.AddDynamic(this, &APDEquipmentModificationLiftActor::HandleEquipmentModificationClosed);
}

void APDEquipmentModificationLiftActor::UnbindFromEquipmentModificationActor()
{
	if (!TargetEquipmentModificationActor)
	{
		return;
	}

	TargetEquipmentModificationActor->OnEquipmentModificationOpened.RemoveAll(this);
	TargetEquipmentModificationActor->OnEquipmentModificationClosed.RemoveAll(this);
}

void APDEquipmentModificationLiftActor::SetRaised(bool bRaised, bool bInstant)
{
	ClearLiftDelay();

	const float Delay = bRaised ? RaiseStartDelay : LowerStartDelay;
	if (bInstant || Delay <= 0.f)
	{
		StartLiftMovement(bRaised, bInstant);
		return;
	}

	bQueuedRaised = bRaised;
	GetWorldTimerManager().SetTimer(LiftDelayTimerHandle, this, &APDEquipmentModificationLiftActor::StartDelayedLiftMovement, Delay, false);
}

void APDEquipmentModificationLiftActor::StartLiftMovement(bool bRaised, bool bInstant)
{
	TargetAlpha = bRaised ? 1.f : 0.f;

	if (bInstant)
	{
		CurrentAlpha = TargetAlpha;
		StartAlpha = TargetAlpha;
		MovementElapsed = 0.f;
		ApplyLiftAlpha(CurrentAlpha);
		SetActorTickEnabled(false);
		return;
	}

	StartAlpha = CurrentAlpha;
	MovementElapsed = 0.f;
	ActiveMovementDuration = FMath::Max(bRaised ? RaiseDuration : LowerDuration, KINDA_SMALL_NUMBER);

	if (FMath::IsNearlyEqual(StartAlpha, TargetAlpha, 0.001f))
	{
		FinishLiftMovement();
		return;
	}

	SetActorTickEnabled(true);
}

void APDEquipmentModificationLiftActor::StartDelayedLiftMovement()
{
	StartLiftMovement(bQueuedRaised, false);
}

void APDEquipmentModificationLiftActor::FinishLiftMovement()
{
	CurrentAlpha = TargetAlpha;
	ApplyLiftAlpha(CurrentAlpha);
	SetActorTickEnabled(false);
}

void APDEquipmentModificationLiftActor::ApplyLiftAlpha(float Alpha)
{
	SetActorLocation(FMath::Lerp(ClosedWorldLocation, ClosedWorldLocation + RaisedWorldOffset, Alpha));
}

void APDEquipmentModificationLiftActor::ClearLiftDelay()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LiftDelayTimerHandle);
	}
}

void APDEquipmentModificationLiftActor::HandleEquipmentModificationOpened(APDEquipmentModificationActor* ModificationActor)
{
	if (ModificationActor == TargetEquipmentModificationActor)
	{
		RaiseActor(false);
	}
}

void APDEquipmentModificationLiftActor::HandleEquipmentModificationClosed(APDEquipmentModificationActor* ModificationActor)
{
	if (ModificationActor == TargetEquipmentModificationActor)
	{
		LowerActor(false);
	}
}
