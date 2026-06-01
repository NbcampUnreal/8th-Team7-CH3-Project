#include "Items/Station/PDEquipDisplayActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Items/Station/PDEquipmentModificationActor.h"
#include "Materials/MaterialInterface.h"

APDEquipDisplayActor::APDEquipDisplayActor()
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

void APDEquipDisplayActor::BeginPlay()
{
	Super::BeginPlay();

	BindToEquipActor();
	TurnOffDisplay();
}

void APDEquipDisplayActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	TurnOffDisplay();
}

void APDEquipDisplayActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromEquipActor();
	Super::EndPlay(EndPlayReason);
}

void APDEquipDisplayActor::TurnOnDisplay()
{
	ApplyDisplayMaterial(OnMaterial.Get());
}

void APDEquipDisplayActor::TurnOffDisplay()
{
	ApplyDisplayMaterial(OffMaterial.Get());
}

void APDEquipDisplayActor::BindToEquipActor()
{
	if (!TargetEquipActor)
	{
		return;
	}

	TargetEquipActor->OnEquipmentModificationOpened.RemoveAll(this);
	TargetEquipActor->OnEquipmentModificationClosed.RemoveAll(this);
	TargetEquipActor->OnEquipmentModificationOpened.AddDynamic(this, &APDEquipDisplayActor::HandleEquipOpened);
	TargetEquipActor->OnEquipmentModificationClosed.AddDynamic(this, &APDEquipDisplayActor::HandleEquipClosed);
}

void APDEquipDisplayActor::UnbindFromEquipActor()
{
	if (!TargetEquipActor)
	{
		return;
	}

	TargetEquipActor->OnEquipmentModificationOpened.RemoveAll(this);
	TargetEquipActor->OnEquipmentModificationClosed.RemoveAll(this);
}

void APDEquipDisplayActor::ApplyDisplayMaterial(UMaterialInterface* Material)
{
	if (!ScreenMesh || !Material)
	{
		return;
	}

	ScreenMesh->SetMaterial(ScreenMaterialElementIndex, Material);
}

void APDEquipDisplayActor::HandleEquipOpened(APDEquipmentModificationActor* EquipActor)
{
	if (EquipActor == TargetEquipActor)
	{
		TurnOnDisplay();
	}
}

void APDEquipDisplayActor::HandleEquipClosed(APDEquipmentModificationActor* EquipActor)
{
	if (EquipActor == TargetEquipActor)
	{
		TurnOffDisplay();
	}
}
