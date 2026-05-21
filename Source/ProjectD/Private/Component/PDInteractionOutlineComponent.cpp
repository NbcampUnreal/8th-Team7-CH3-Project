#include "Component/PDInteractionOutlineComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"

UPDInteractionOutlineComponent::UPDInteractionOutlineComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDInteractionOutlineComponent::BeginPlay()
{
	Super::BeginPlay();

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

void UPDInteractionOutlineComponent::SetOutlineEnabled(bool bEnabled)
{
	if (bOutlineEnabled == bEnabled)
	{
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	CachePrimitiveRenderState(PrimitiveComponents);

	if (bEnabled)
	{
		PreviousCustomDepthStates.Reset();
		PreviousStencilValues.Reset();

		for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
		{
			if (!PrimitiveComponent || PrimitiveComponent == TriggerComponent)
			{
				continue;
			}

			PreviousCustomDepthStates.Add(PrimitiveComponent, PrimitiveComponent->bRenderCustomDepth);
			PreviousStencilValues.Add(PrimitiveComponent, PrimitiveComponent->CustomDepthStencilValue);
			PrimitiveComponent->SetRenderCustomDepth(true);
			PrimitiveComponent->SetCustomDepthStencilValue(StencilValue);
		}
	}
	else
	{
		for (const TPair<TWeakObjectPtr<UPrimitiveComponent>, bool>& Pair : PreviousCustomDepthStates)
		{
			UPrimitiveComponent* PrimitiveComponent = Pair.Key.Get();
			if (!PrimitiveComponent)
			{
				continue;
			}

			PrimitiveComponent->SetRenderCustomDepth(Pair.Value);

			if (const int32* PreviousStencilValue = PreviousStencilValues.Find(PrimitiveComponent))
			{
				PrimitiveComponent->SetCustomDepthStencilValue(*PreviousStencilValue);
			}
		}

		PreviousCustomDepthStates.Reset();
		PreviousStencilValues.Reset();
	}

	bOutlineEnabled = bEnabled;
}

void UPDInteractionOutlineComponent::CachePrimitiveRenderState(TArray<UPrimitiveComponent*>& OutComponents) const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	Owner->GetComponents<UPrimitiveComponent>(OutComponents, bApplyToChildActors);
}

void UPDInteractionOutlineComponent::ResetOverlapState()
{
	OverlappingPawns.Reset();
	SetOutlineEnabled(false);
}
