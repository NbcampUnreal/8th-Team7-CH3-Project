#include "Items/Station/PDStashDisplayActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Items/Station/PDStashActor.h"
#include "Materials/MaterialInterface.h"

APDStashDisplayActor::APDStashDisplayActor()
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

void APDStashDisplayActor::BeginPlay()
{
	Super::BeginPlay();

	BindToStashActor();
	TurnOffDisplay();
}

void APDStashDisplayActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	TurnOffDisplay();
}

void APDStashDisplayActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromStashActor();
	Super::EndPlay(EndPlayReason);
}

void APDStashDisplayActor::TurnOnDisplay()
{
	ApplyDisplayMaterial(OnMaterial.Get());
}

void APDStashDisplayActor::TurnOffDisplay()
{
	ApplyDisplayMaterial(OffMaterial.Get());
}

void APDStashDisplayActor::BindToStashActor()
{
	if (!TargetStashActor)
	{
		return;
	}

	TargetStashActor->OnStorageOpened.RemoveAll(this);
	TargetStashActor->OnStorageClosed.RemoveAll(this);
	TargetStashActor->OnStorageOpened.AddDynamic(this, &APDStashDisplayActor::HandleStashOpened);
	TargetStashActor->OnStorageClosed.AddDynamic(this, &APDStashDisplayActor::HandleStashClosed);
}

void APDStashDisplayActor::UnbindFromStashActor()
{
	if (!TargetStashActor)
	{
		return;
	}

	TargetStashActor->OnStorageOpened.RemoveAll(this);
	TargetStashActor->OnStorageClosed.RemoveAll(this);
}

void APDStashDisplayActor::ApplyDisplayMaterial(UMaterialInterface* Material)
{
	if (!ScreenMesh || !Material)
	{
		return;
	}

	ScreenMesh->SetMaterial(ScreenMaterialElementIndex, Material);
}

void APDStashDisplayActor::HandleStashOpened(APDStashActor* StashActor)
{
	if (StashActor == TargetStashActor)
	{
		TurnOnDisplay();
	}
}

void APDStashDisplayActor::HandleStashClosed(APDStashActor* StashActor)
{
	if (StashActor == TargetStashActor)
	{
		TurnOffDisplay();
	}
}
