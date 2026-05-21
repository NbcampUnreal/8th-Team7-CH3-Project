#include "Occlusion/PDBuildingFloorVolume.h"

#include "Components/BoxComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Pawn.h"
#include "Occlusion/PDFloorDetectionComponent.h"
#include "Occlusion/PDFloorOcclusionComponent.h"

APDBuildingFloorVolume::APDBuildingFloorVolume()
{
    PrimaryActorTick.bCanEverTick = false;

    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    RootComponent = TriggerBox;
    TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerBox->SetCollisionResponseToAllChannels(ECR_Overlap);
    TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    TriggerBox->SetGenerateOverlapEvents(true);
}

void APDBuildingFloorVolume::BeginPlay()
{
    Super::BeginPlay();

    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APDBuildingFloorVolume::HandleBeginOverlap);
    TriggerBox->OnComponentEndOverlap.AddDynamic(this, &APDBuildingFloorVolume::HandleEndOverlap);

    AttachComponentsToOverlappingMeshes();
}

void APDBuildingFloorVolume::AttachComponentsToOverlappingMeshes()
{
    if (!TriggerBox)
    {
        return;
    }

    // 박스 영역 내 모든 StaticMeshActor 스캔
    TArray<AActor*> Overlapping;
    TriggerBox->GetOverlappingActors(Overlapping, AStaticMeshActor::StaticClass());

    for (AActor* MeshActor : Overlapping)
    {
        if (!MeshActor)
        {
            continue;
        }

        // 이미 다른 볼륨이 등록한 메시는 스킵 (한 메시는 한 층에만 속함)
        if (MeshActor->FindComponentByClass<UPDFloorOcclusionComponent>())
        {
            continue;
        }

        UPDFloorOcclusionComponent* NewComp = NewObject<UPDFloorOcclusionComponent>(MeshActor);
        if (!NewComp)
        {
            continue;
        }

        NewComp->InitializeFloorInfo(BuildingGroupID, FloorLevel);
        NewComp->RegisterComponent();
    }
}

void APDBuildingFloorVolume::HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    APawn* OverlappingPawn = Cast<APawn>(OtherActor);
    if (!OverlappingPawn || !OverlappingPawn->IsLocallyControlled())
    {
        return;
    }

    UPDFloorDetectionComponent* Detection = OverlappingPawn->FindComponentByClass<UPDFloorDetectionComponent>();
    if (Detection)
    {
        Detection->OnEnteredBuildingFloor(BuildingGroupID, FloorLevel);
    }

    // 캐릭터 진입 시점에 WP로 늦게 로드된 메시 다시 스캔
    AttachComponentsToOverlappingMeshes();
}

void APDBuildingFloorVolume::HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    APawn* OverlappingPawn = Cast<APawn>(OtherActor);
    if (!OverlappingPawn || !OverlappingPawn->IsLocallyControlled())
    {
        return;
    }

    UPDFloorDetectionComponent* Detection = OverlappingPawn->FindComponentByClass<UPDFloorDetectionComponent>();
    if (Detection)
    {
        Detection->OnLeftBuildingFloor(BuildingGroupID, FloorLevel);
    }
}