#include "Items/Station/PDEquipmentModificationDisplayActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Items/Station/PDEquipmentModificationActor.h"
#include "Materials/MaterialInterface.h"
#include "UObject/NameTypes.h"

APDEquipmentModificationDisplayActor::APDEquipmentModificationDisplayActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DisplayMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DisplayMesh"));
	DisplayMesh->SetupAttachment(SceneRoot);
	DisplayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APDEquipmentModificationDisplayActor::BeginPlay()
{
	Super::BeginPlay();

	BindToEquipmentModificationActor();
	TurnOffDisplay();
}

void APDEquipmentModificationDisplayActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	TurnOffDisplay();
}

void APDEquipmentModificationDisplayActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromEquipmentModificationActor();
	Super::EndPlay(EndPlayReason);
}

void APDEquipmentModificationDisplayActor::TurnOnDisplay()
{
	ApplyDisplayMaterial(OnMaterial.Get());
}

void APDEquipmentModificationDisplayActor::TurnOffDisplay()
{
	ApplyDisplayMaterial(OffMaterial.Get());
}

void APDEquipmentModificationDisplayActor::BindToEquipmentModificationActor()
{
	if (!TargetEquipmentModificationActor)
	{
		return;
	}

	TargetEquipmentModificationActor->OnEquipmentModificationOpened.RemoveAll(this);
	TargetEquipmentModificationActor->OnEquipmentModificationClosed.RemoveAll(this);
	TargetEquipmentModificationActor->OnEquipmentModificationOpened.AddDynamic(this, &APDEquipmentModificationDisplayActor::HandleEquipmentModificationOpened);
	TargetEquipmentModificationActor->OnEquipmentModificationClosed.AddDynamic(this, &APDEquipmentModificationDisplayActor::HandleEquipmentModificationClosed);
}

void APDEquipmentModificationDisplayActor::UnbindFromEquipmentModificationActor()
{
	if (!TargetEquipmentModificationActor)
	{
		return;
	}

	TargetEquipmentModificationActor->OnEquipmentModificationOpened.RemoveAll(this);
	TargetEquipmentModificationActor->OnEquipmentModificationClosed.RemoveAll(this);
}

UStaticMeshComponent* APDEquipmentModificationDisplayActor::GetTargetMeshComponent() const
{
	if (TargetMeshComponentName.IsNone())
	{
		return DisplayMesh.Get();
	}

	TArray<UStaticMeshComponent*> StaticMeshComponents;
	GetComponents<UStaticMeshComponent>(StaticMeshComponents);

	for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
	{
		if (!StaticMeshComponent)
		{
			continue;
		}

		if (StaticMeshComponent->GetFName() == TargetMeshComponentName || StaticMeshComponent->GetName() == TargetMeshComponentName.ToString())
		{
			return StaticMeshComponent;
		}
	}

	return DisplayMesh.Get();
}

void APDEquipmentModificationDisplayActor::ApplyDisplayMaterial(UMaterialInterface* Material)
{
	UStaticMeshComponent* TargetMeshComponent = GetTargetMeshComponent();

	if (!TargetMeshComponent || !Material)
	{
		return;
	}

	TargetMeshComponent->SetMaterial(MaterialElementIndex, Material);
}

void APDEquipmentModificationDisplayActor::HandleEquipmentModificationOpened(APDEquipmentModificationActor* ModificationActor)
{
	if (ModificationActor == TargetEquipmentModificationActor)
	{
		TurnOnDisplay();
	}
}

void APDEquipmentModificationDisplayActor::HandleEquipmentModificationClosed(APDEquipmentModificationActor* ModificationActor)
{
	if (ModificationActor == TargetEquipmentModificationActor)
	{
		TurnOffDisplay();
	}
}
