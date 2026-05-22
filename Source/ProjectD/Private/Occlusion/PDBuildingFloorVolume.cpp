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
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &APDBuildingFloorVolume::CheckInitialPawnOverlap, 0.1f, false);
    
    TArray<AActor*> Overlapping;
    TriggerBox->GetOverlappingActors(Overlapping, AStaticMeshActor::StaticClass());
    UE_LOG(LogTemp, Warning, TEXT("[FloorVolume] %s Floor=%d : 박스 내 메시 %d개"),
        *BuildingGroupID.ToString(), FloorLevel, Overlapping.Num());
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

    //박스 영역으로 직접 overlap query.
    //GetOverlappingActors는 컴포넌트의 OverlapEvents에 의존하지만,
    //World->OverlapMulti는 콜리전 채널만 보고 query하므로 메시쪽 설정 무관.
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

    UE_LOG(LogTemp, Warning, TEXT("[FloorVolume] %s Floor=%d Overlap query 결과 %d개"),
        *BuildingGroupID.ToString(), FloorLevel, Overlaps.Num());

    for (const FOverlapResult& Result : Overlaps)
    {
        AActor* MeshActor = Result.GetActor();
        if (!MeshActor)
        {
            continue;
        }

        //StaticMeshActor만 잡기. BP 액터에 자식 메시가 있는 경우는 별도 대응 필요.
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

        UE_LOG(LogTemp, Warning, TEXT("[FloorVolume]   부착: %s"), *MeshActor->GetName());
    }
}

void APDBuildingFloorVolume::HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{   
    UE_LOG(LogTemp, Warning, TEXT("[FloorVolume] %s Floor=%d : %s 진입"),
        *BuildingGroupID.ToString(), FloorLevel, *GetNameSafe(OtherActor));
    
    APawn* OverlappingPawn = Cast<APawn>(OtherActor);
    if (!OverlappingPawn || !OverlappingPawn->IsLocallyControlled())
    {   
        UE_LOG(LogTemp, Warning, TEXT("[FloorVolume] Pawn 아님 또는 LocallyControlled 아님"));
        return;
    }

    UPDFloorDetectionComponent* Detection = OverlappingPawn->FindComponentByClass<UPDFloorDetectionComponent>();
    if (Detection)
    {
        UE_LOG(LogTemp, Warning, TEXT("[FloorVolume] Detection 찾음, 알림 전송"));
        Detection->OnEnteredBuildingFloor(BuildingGroupID, FloorLevel);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[FloorVolume] Detection 컴포넌트 없음! 캐릭터 BP에 추가 필요"));
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

        UPDFloorDetectionComponent* Detection = OverlappingPawn->FindComponentByClass<UPDFloorDetectionComponent>();
        if (Detection)
        {
            Detection->OnEnteredBuildingFloor(BuildingGroupID, FloorLevel);
            UE_LOG(LogTemp, Warning, TEXT("[FloorVolume] %s Floor=%d 시작 시 캐릭터 이미 내부"),
                *BuildingGroupID.ToString(), FloorLevel);
        }
    }
}