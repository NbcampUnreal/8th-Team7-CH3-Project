#include "Items/PDMarketDoorActor.h"

#include "Components/SceneComponent.h"
#include "Items/PDMarketActor.h"

APDMarketDoorActor::APDMarketDoorActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void APDMarketDoorActor::BeginPlay()
{
	Super::BeginPlay();

	TArray<USceneComponent*> Components;
	GetComponents<USceneComponent>(Components);

	for (USceneComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		if (Component->GetFName() == UpperDoorComponentName)
		{
			UpperDoor = Component;
			UpperClosed = Component->GetRelativeLocation();
		}
		else if (Component->GetFName() == LowerDoorComponentName)
		{
			LowerDoor = Component;
			LowerClosed = Component->GetRelativeLocation();
		}
	}

	Apply();
	Bind();
}

void APDMarketDoorActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Unbind();
	Super::EndPlay(EndPlayReason);
}

void APDMarketDoorActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const bool bClosing = TargetAlpha < CurrentAlpha;
	const float PreviousAlpha = CurrentAlpha;
	const float MoveSpeed = bClosing ? CloseMoveSpeed : OpenMoveSpeed;

	CurrentAlpha = FMath::FInterpTo(CurrentAlpha, TargetAlpha, DeltaSeconds, MoveSpeed);

	if (bClosing && MinimumCloseAlphaSpeed > 0.f && PreviousAlpha > TargetAlpha)
	{
		const float MinimumStep = MinimumCloseAlphaSpeed * DeltaSeconds;
		CurrentAlpha = FMath::Min(CurrentAlpha, PreviousAlpha - MinimumStep);
		CurrentAlpha = FMath::Max(CurrentAlpha, TargetAlpha);
	}

	if (FMath::Abs(CurrentAlpha - TargetAlpha) <= FinishAlphaTolerance)
	{
		CurrentAlpha = TargetAlpha;
	}

	Apply();

	if (FMath::IsNearlyEqual(CurrentAlpha, TargetAlpha, KINDA_SMALL_NUMBER))
	{
		SetActorTickEnabled(false);
	}
}

void APDMarketDoorActor::Apply()
{
	if (UpperDoor.IsValid())
	{
		UpperDoor->SetRelativeLocation(FMath::Lerp(UpperClosed, UpperClosed + UpperOpenOffset, CurrentAlpha));
	}

	if (LowerDoor.IsValid())
	{
		LowerDoor->SetRelativeLocation(FMath::Lerp(LowerClosed, LowerClosed + LowerOpenOffset, CurrentAlpha));
	}
}

void APDMarketDoorActor::Bind()
{
	if (!TargetMarketActor)
	{
		return;
	}

	TargetMarketActor->OnMarketOpened.RemoveAll(this);
	TargetMarketActor->OnMarketClosed.RemoveAll(this);
	TargetMarketActor->OnMarketOpened.AddDynamic(this, &APDMarketDoorActor::HandleOpened);
	TargetMarketActor->OnMarketClosed.AddDynamic(this, &APDMarketDoorActor::HandleClosed);
}

void APDMarketDoorActor::Unbind()
{
	if (!TargetMarketActor)
	{
		return;
	}

	TargetMarketActor->OnMarketOpened.RemoveAll(this);
	TargetMarketActor->OnMarketClosed.RemoveAll(this);
}

void APDMarketDoorActor::HandleOpened(APDMarketActor* Actor)
{
	if (Actor == TargetMarketActor)
	{
		TargetAlpha = 1.f;
		SetActorTickEnabled(true);
	}
}

void APDMarketDoorActor::HandleClosed(APDMarketActor* Actor)
{
	if (Actor == TargetMarketActor)
	{
		TargetAlpha = 0.f;
		SetActorTickEnabled(true);
	}
}
