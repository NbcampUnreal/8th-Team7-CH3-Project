#include "Items/Market/PDMarketDisplayActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Items/Market/PDMarketActor.h"
#include "Materials/MaterialInterface.h"

APDMarketDisplayActor::APDMarketDisplayActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(SceneRoot);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ScreenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScreenMesh"));
	ScreenMesh->SetupAttachment(SceneRoot);
	ScreenMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APDMarketDisplayActor::BeginPlay()
{
	Super::BeginPlay();

	BindToMarketActor();
	TurnOffDisplay();
}

void APDMarketDisplayActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	TurnOffDisplay();
}

void APDMarketDisplayActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromMarketActor();
	Super::EndPlay(EndPlayReason);
}

void APDMarketDisplayActor::TurnOnDisplay()
{
	ApplyDisplayMaterial(OnMaterial.Get());
}

void APDMarketDisplayActor::TurnOffDisplay()
{
	ApplyDisplayMaterial(OffMaterial.Get());
}

void APDMarketDisplayActor::BindToMarketActor()
{
	if (!TargetMarketActor)
	{
		return;
	}

	TargetMarketActor->OnMarketOpened.RemoveAll(this);
	TargetMarketActor->OnMarketClosed.RemoveAll(this);
	TargetMarketActor->OnMarketOpened.AddDynamic(this, &APDMarketDisplayActor::HandleMarketOpened);
	TargetMarketActor->OnMarketClosed.AddDynamic(this, &APDMarketDisplayActor::HandleMarketClosed);
}

void APDMarketDisplayActor::UnbindFromMarketActor()
{
	if (!TargetMarketActor)
	{
		return;
	}

	TargetMarketActor->OnMarketOpened.RemoveAll(this);
	TargetMarketActor->OnMarketClosed.RemoveAll(this);
}

void APDMarketDisplayActor::ApplyDisplayMaterial(UMaterialInterface* Material)
{
	if (!ScreenMesh || !Material)
	{
		return;
	}

	ScreenMesh->SetMaterial(ScreenMaterialElementIndex, Material);
}

void APDMarketDisplayActor::HandleMarketOpened(APDMarketActor* MarketActor)
{
	if (MarketActor == TargetMarketActor)
	{
		TurnOnDisplay();
	}
}

void APDMarketDisplayActor::HandleMarketClosed(APDMarketActor* MarketActor)
{
	if (MarketActor == TargetMarketActor)
	{
		TurnOffDisplay();
	}
}
