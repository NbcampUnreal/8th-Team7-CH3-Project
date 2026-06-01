#include "Occlusion/PDBuildingFloorVolume.h"
#include "Engine/OverlapResult.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
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

    // 시작 시 박스 안에 이미 있는 캐릭터 직접 체크 (BeginOverlap 이벤트 안 트리거되는 케이스)
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &APDBuildingFloorVolume::CheckInitialPawnOverlap, 0.1f, false);
} 

void APDBuildingFloorVolume::AttachComponentsToOverlappingMeshes()
{
    if (!TriggerBox)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    TArray<FOverlapResult> Overlaps;
    const FCollisionShape BoxShape = FCollisionShape::MakeBox(TriggerBox->GetScaledBoxExtent());

    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
    ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

    World->OverlapMultiByObjectType(
        Overlaps,
        TriggerBox->GetComponentLocation(),
        TriggerBox->GetComponentQuat(),
        ObjectParams,
        BoxShape
    );

    for (const FOverlapResult& Result : Overlaps)
    {
        AActor* MeshActor = Result.GetActor();
        if (!MeshActor)
        {
            continue;
        }

        if (!MeshActor->IsA(AStaticMeshActor::StaticClass()))
        {
            continue;
        }

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

    if (UPDFloorDetectionComponent* Detection = OverlappingPawn->FindComponentByClass<UPDFloorDetectionComponent>())
    {
        Detection->OnEnteredBuildingFloor(BuildingGroupID, FloorLevel);
    }

    //캐릭터 진입 시점에 WP로 늦게 로드된 메시 다시 스캔
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

void APDBuildingFloorVolume::CheckInitialPawnOverlap()
{
    UWorld* World = GetWorld();
    if (!World || !TriggerBox)
    {
        return;
    }

    TArray<FOverlapResult> Overlaps;
    const FCollisionShape BoxShape = FCollisionShape::MakeBox(TriggerBox->GetScaledBoxExtent());

    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

    World->OverlapMultiByObjectType(
        Overlaps,
        TriggerBox->GetComponentLocation(),
        TriggerBox->GetComponentQuat(),
        ObjectParams,
        BoxShape
    );

    for (const FOverlapResult& Result : Overlaps)
    {
        APawn* OverlappingPawn = Cast<APawn>(Result.GetActor());
        if (!OverlappingPawn || !OverlappingPawn->IsLocallyControlled())
        {
            continue;
        }

        if (UPDFloorDetectionComponent* Detection = OverlappingPawn->FindComponentByClass<UPDFloorDetectionComponent>())
        {
            Detection->OnEnteredBuildingFloor(BuildingGroupID, FloorLevel);
        }
    }
}