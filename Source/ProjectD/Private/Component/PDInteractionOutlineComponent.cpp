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

	BindTrigger();
	RefreshOverlapState();
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

void UPDInteractionOutlineComponent::SetOverlapTriggerEnabled(bool bEnabled)
{
	if (bEnableOverlapTrigger == bEnabled)
	{
		return;
	}

	bEnableOverlapTrigger = bEnabled;

	if (!bEnableOverlapTrigger)
	{
		UnbindTrigger();
		ResetOverlapState();
		return;
	}

	BindTrigger();
	RefreshOverlapState();
}

void UPDInteractionOutlineComponent::BindTrigger()
{
	if (!bEnableOverlapTrigger || bTriggerBound || !TriggerComponent)
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
	PruneInvalidOverlaps();
	SetOutlineEnabled(OverlappingPawns.Num() > 0);
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

	if (bOutlineEnabled)
	{
		ApplyOverlayMaterial();
	}
	else
	{
		RemoveOverlayMaterial();
	}

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

	TArray<UMeshComponent*> MeshComponents;
	CacheMeshComponents(MeshComponents);

	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!IsValid(MeshComponent))
		{
			continue;
		}

		if (!PreviousOverlayMaterials.Contains(MeshComponent))
		{
			PreviousOverlayMaterials.Add(MeshComponent, MeshComponent->GetOverlayMaterial());
		}

		MeshComponent->SetOverlayMaterial(OutlineMID);
	}
}

void UPDInteractionOutlineComponent::RemoveOverlayMaterial()
{
	for (auto It = PreviousOverlayMaterials.CreateIterator(); It; ++It)
	{
		UMeshComponent* MeshComponent = It.Key().Get();
		if (!IsValid(MeshComponent))
		{
			It.RemoveCurrent();
			continue;
		}

		MeshComponent->SetOverlayMaterial(It.Value().Get());
		It.RemoveCurrent();
	}
}

void UPDInteractionOutlineComponent::RefreshOverlapState()
{
	OverlappingPawns.Reset();

	if (!bEnableOverlapTrigger || !TriggerComponent)
	{
		SetOutlineEnabled(false);
		return;
	}

	TArray<AActor*> OverlappingActors;
	TriggerComponent->GetOverlappingActors(OverlappingActors, APawn::StaticClass());

	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (IsValidInteractor(OverlappingActor))
		{
			OverlappingPawns.Add(CastChecked<APawn>(OverlappingActor));
		}
	}

	SetOutlineEnabled(OverlappingPawns.Num() > 0);
}

void UPDInteractionOutlineComponent::PruneInvalidOverlaps()
{
	for (auto It = OverlappingPawns.CreateIterator(); It; ++It)
	{
		APawn* Pawn = It->Get();
		if (!IsValid(Pawn) || !IsValidInteractor(Pawn))
		{
			It.RemoveCurrent();
		}
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
