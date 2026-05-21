#include "Occlusion/PDFloorOcclusionComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
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

	// === 폴리싱 마이그레이션 포인트 시작 ===
	// 1차: 양자택일. 0.5 미만이면 SetActorHiddenInGame(true).
	// 2차: BeginPlay에서 메시 컴포넌트들의 머티리얼을 DMI로 캐시.
	//      여기서 DMI에 SetScalarParameterValue("DitherFade", CurrentFadeAmount) 호출.
	//      alpha == 0 도달 시에만 SetActorHiddenInGame(true) (충돌 비용 절감).
	const bool bShouldBeHidden = CurrentFadeAmount < 0.5f;
	if (Owner->IsHidden() != bShouldBeHidden)
	{
		Owner->SetActorHiddenInGame(bShouldBeHidden);
	}
	// === 폴리싱 마이그레이션 포인트 끝 ===
}