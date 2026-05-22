#include "Component/PDInteractionOutlineComponent.h"

#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

UPDInteractionOutlineComponent::UPDInteractionOutlineComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDInteractionOutlineComponent::BeginPlay()
{
	Super::BeginPlay();

	ApplyOverlayMaterial();
	BindTrigger();
}

void UPDInteractionOutlineComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindTrigger();
	ResetOverlapState();

	Super::EndPlay(EndPlayReason);
}

void UPDInteractionOutlineComponent::SetupTrigger(UPrimitiveComponent* InTriggerComponent)
{
	if (TriggerComponent == InTriggerComponent)
	{
		return;
	}

	UnbindTrigger();
	TriggerComponent = InTriggerComponent;

	const AActor* Owner = GetOwner();
	if (Owner && Owner->HasActorBegunPlay())
	{
		BindTrigger();
	}
}

void UPDInteractionOutlineComponent::BindTrigger()
{
	if (bTriggerBound || !TriggerComponent)
	{
		return;
	}

	TriggerComponent->OnComponentBeginOverlap.AddDynamic(this, &UPDInteractionOutlineComponent::HandleTriggerBeginOverlap);
	TriggerComponent->OnComponentEndOverlap.AddDynamic(this, &UPDInteractionOutlineComponent::HandleTriggerEndOverlap);
	bTriggerBound = true;
}

void UPDInteractionOutlineComponent::UnbindTrigger()
{
	if (!bTriggerBound || !TriggerComponent)
	{
		bTriggerBound = false;
		return;
	}

	TriggerComponent->OnComponentBeginOverlap.RemoveDynamic(this, &UPDInteractionOutlineComponent::HandleTriggerBeginOverlap);
	TriggerComponent->OnComponentEndOverlap.RemoveDynamic(this, &UPDInteractionOutlineComponent::HandleTriggerEndOverlap);
	bTriggerBound = false;
}

void UPDInteractionOutlineComponent::HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValidInteractor(OtherActor))
	{
		return;
	}

	OverlappingPawns.Add(CastChecked<APawn>(OtherActor));
	SetOutlineEnabled(true);
}

void UPDInteractionOutlineComponent::HandleTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	OverlappingPawns.Remove(Pawn);

	if (OverlappingPawns.Num() == 0)
	{
		SetOutlineEnabled(false);
	}
}

bool UPDInteractionOutlineComponent::IsValidInteractor(AActor* Actor) const
{
	APawn* Pawn = Cast<APawn>(Actor);
	if (!Pawn)
	{
		return false;
	}

	return !bOnlyLocalPlayer || Pawn->IsLocallyControlled();
}

void UPDInteractionOutlineComponent::RefreshOutlineTargets()
{
	ApplyOverlayMaterial();
}

void UPDInteractionOutlineComponent::SetOutlineEnabled(bool bEnabled)
{
	if (bOutlineEnabled == bEnabled)
	{
		return;
	}

	bOutlineEnabled = bEnabled;
	UpdateOutlineParameters();
}

void UPDInteractionOutlineComponent::ApplyOverlayMaterial()
{
	if (!OutlineMaterial)
	{
		return;
	}

	if (!OutlineMID)
	{
		OutlineMID = UMaterialInstanceDynamic::Create(OutlineMaterial, this);
	}

	if (!OutlineMID)
	{
		return;
	}

	UpdateOutlineParameters();

	TArray<UMeshComponent*> MeshComponents;
	CacheMeshComponents(MeshComponents);

	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		MeshComponent->SetOverlayMaterial(OutlineMID);
	}
}

void UPDInteractionOutlineComponent::UpdateOutlineParameters()
{
	if (!OutlineMID)
	{
		return;
	}

	OutlineMID->SetScalarParameterValue(TEXT("OLIntensity"), bOutlineEnabled ? OutlineIntensity : 0.f);
	OutlineMID->SetScalarParameterValue(TEXT("OLThickness"), OutlineThickness);
}

void UPDInteractionOutlineComponent::CacheMeshComponents(TArray<UMeshComponent*>& OutComponents) const
{
	OutComponents.Reset();

	TSet<TWeakObjectPtr<UMeshComponent>> AddedComponents;

	bool bHasExplicitTarget = false;
	for (AActor* TargetActor : OutlineTargetActors)
	{
		if (!IsValid(TargetActor))
		{
			continue;
		}

		bHasExplicitTarget = true;
		AppendActorMeshComponents(TargetActor, OutComponents, AddedComponents);
	}

	if (!bHasExplicitTarget)
	{
		AppendActorMeshComponents(GetOwner(), OutComponents, AddedComponents);
	}
}

void UPDInteractionOutlineComponent::AppendActorMeshComponents(AActor* Actor, TArray<UMeshComponent*>& OutComponents, TSet<TWeakObjectPtr<UMeshComponent>>& AddedComponents) const
{
	if (!IsValid(Actor))
	{
		return;
	}

	TArray<UMeshComponent*> MeshComponents;
	Actor->GetComponents<UMeshComponent>(MeshComponents, bApplyToChildActors);

	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!IsValid(MeshComponent) || AddedComponents.Contains(MeshComponent))
		{
			continue;
		}

		AddedComponents.Add(MeshComponent);
		OutComponents.Add(MeshComponent);
	}
}

void UPDInteractionOutlineComponent::ResetOverlapState()
{
	OverlappingPawns.Reset();
	SetOutlineEnabled(false);
}
