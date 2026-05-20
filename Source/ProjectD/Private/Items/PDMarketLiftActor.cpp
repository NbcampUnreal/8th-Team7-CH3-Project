#include "Items/PDMarketLiftActor.h"

#include "Components/SceneComponent.h"
#include "Items/PDMarketActor.h"
#include "TimerManager.h"

APDMarketLiftActor::APDMarketLiftActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void APDMarketLiftActor::BeginPlay()
{
	Super::BeginPlay();

	ClosedLocation = GetActorLocation();
	TargetLocation = ClosedLocation;
	BindToMarketActor();
	MoveDown(true);
}

void APDMarketLiftActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromMarketActor();
	Super::EndPlay(EndPlayReason);
}

void APDMarketLiftActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaSeconds, MoveInterpSpeed);
	SetActorLocation(NewLocation);

	if (FVector::DistSquared(NewLocation, TargetLocation) <= 1.f)
	{
		SetActorLocation(TargetLocation);
		SetActorTickEnabled(false);
	}
}

void APDMarketLiftActor::MoveUp(bool bInstant)
{
	TargetLocation = ClosedLocation + OpenLocationOffset;
	MoveToTargetLocation(bInstant);
}

void APDMarketLiftActor::MoveDown(bool bInstant)
{
	TargetLocation = ClosedLocation;
	MoveToTargetLocation(bInstant);
}

void APDMarketLiftActor::BindToMarketActor()
{
	if (!TargetMarketActor)
	{
		return;
	}

	TargetMarketActor->OnMarketOpened.RemoveAll(this);
	TargetMarketActor->OnMarketClosed.RemoveAll(this);
	TargetMarketActor->OnMarketOpened.AddDynamic(this, &APDMarketLiftActor::HandleMarketOpened);
	TargetMarketActor->OnMarketClosed.AddDynamic(this, &APDMarketLiftActor::HandleMarketClosed);
}

void APDMarketLiftActor::UnbindFromMarketActor()
{
	if (!TargetMarketActor)
	{
		return;
	}

	TargetMarketActor->OnMarketOpened.RemoveAll(this);
	TargetMarketActor->OnMarketClosed.RemoveAll(this);
}

void APDMarketLiftActor::MoveToTargetLocation(bool bInstant)
{
	if (bInstant)
	{
		SetActorLocation(TargetLocation);
		SetActorTickEnabled(false);
		return;
	}

	SetActorTickEnabled(true);
}

void APDMarketLiftActor::HandleMarketOpened(APDMarketActor* MarketActor)
{
	if (MarketActor == TargetMarketActor)
	{
		if (OpenDelay <= 0.f)
		{
			MoveUp(false);
		}
		else
		{
			GetWorldTimerManager().SetTimer(OpenDelayTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				MoveUp(false);
			}), OpenDelay, false);
		}
	}
}

void APDMarketLiftActor::HandleMarketClosed(APDMarketActor* MarketActor)
{
	if (MarketActor == TargetMarketActor)
	{
		GetWorldTimerManager().ClearTimer(OpenDelayTimerHandle);
		MoveDown(false);
	}
}
