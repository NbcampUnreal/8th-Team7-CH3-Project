#include "Occlusion/PDFloorOcclusionComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Occlusion/PDFloorOcclusionSubsystem.h"

UPDFloorOcclusionComponent::UPDFloorOcclusionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDFloorOcclusionComponent::InitializeFloorInfo(FName InBuildingGroupID, int32 InFloorLevel)
{
	BuildingGroupID = InBuildingGroupID;
	FloorLevel = InFloorLevel;
}

void UPDFloorOcclusionComponent::BeginPlay()
{
	Super::BeginPlay();

	//Owner의 모든 PrimitiveComponent 머티리얼을 DMI로 교체 + 캐시
	if (AActor* Owner = GetOwner())
	{
		TArray<UPrimitiveComponent*> PrimComponents;
		Owner->GetComponents<UPrimitiveComponent>(PrimComponents);

		for (UPrimitiveComponent* Prim : PrimComponents)
		{
			if (!Prim)
			{
				continue;
			}

			const int32 NumMaterials = Prim->GetNumMaterials();
			for (int32 i = 0; i < NumMaterials; i++)
			{
				UMaterialInstanceDynamic* DynMat = Prim->CreateAndSetMaterialInstanceDynamic(i);
				if (DynMat)
				{
					DitherMaterials.Add(DynMat);
				}
			}
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (UPDFloorOcclusionSubsystem* Subsystem = World->GetSubsystem<UPDFloorOcclusionSubsystem>())
		{
			Subsystem->RegisterFloorActor(this);
			bRegistered = true;
		}
	}
}

void UPDFloorOcclusionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bRegistered)
	{
		if (UWorld* World = GetWorld())
		{
			if (UPDFloorOcclusionSubsystem* Subsystem = World->GetSubsystem<UPDFloorOcclusionSubsystem>())
			{
				Subsystem->UnregisterFloorActor(this);
			}
		}
		bRegistered = false;
	}
	
	Super::EndPlay(EndPlayReason);
}

void UPDFloorOcclusionComponent::SetFadeAmount(float Alpha)
{
	CurrentFadeAmount = FMath::Clamp(Alpha, 0.0f, 1.0f);

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// DMI의 DitherFade Scalar Parameter 갱신 => 머티리얼이 DitherTemporalAA로 부드럽게 discard
	for (UMaterialInstanceDynamic* Mat : DitherMaterials)
	{
		if (Mat)
		{
			Mat->SetScalarParameterValue(TEXT("DitherFade"), CurrentFadeAmount);
		}
	}

	// 알파 0 도달 시에만 hidden 처리 (충돌 비용 절감)
	const bool bShouldBeHidden = FMath::IsNearlyZero(CurrentFadeAmount);
	if (Owner->IsHidden() != bShouldBeHidden)
	{
		Owner->SetActorHiddenInGame(bShouldBeHidden);
	}
}