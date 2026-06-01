#include "Ping/PDPingSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Ping/PDPingMarker.h"
#include "Characters/Base/PDEnemyBase.h"
#include "Enemy/Types/EnemyTypes.h"
#include "Component/PDVisionComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

void UPDPingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UWorld* World = GetWorld())
    {
        //핑 만료 검사
        World->GetTimerManager().SetTimer(
            ExpireTimerHandle, this, &UPDPingSubsystem::TickExpiration,
            1.0f, true); 
    }
}

void UPDPingSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ExpireTimerHandle);
        World->GetTimerManager().ClearTimer(FaintMarkCheckTimer);
    }
    ClearAllPings();
    ClearAllFaintMarks();
    Super::Deinitialize();
}

bool UPDPingSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

int32 UPDPingSubsystem::SpawnPing(EPDPingType InType, const FVector& InWorldLocation)
{
    UWorld* World = GetWorld();
    if (!World) return -1;

    //최대 개수 도달 시 가장 오래된 핑 제거
    if (ActivePings.Num() >= MaxActivePings)
    {
        int32 OldestId = INT_MAX;
        for (const TPair<int32, FPDPingData>& Pair : ActivePings)
        {
            if (Pair.Key < OldestId)
            {
                OldestId = Pair.Key;
            }
        }
        if (OldestId != INT_MAX)
        {
            RemovePing(OldestId);
        }
    }

    FPDPingData Data;
    Data.PingId = NextPingId++;
    Data.PingType = InType;
    Data.WorldLocation = InWorldLocation;
    Data.ExpireTime = (DefaultPingLifetime > 0.f) ? World->GetTimeSeconds() + DefaultPingLifetime : 0.f;

    if (UClass* MarkerClass = DefaultMarkerClass)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (APDPingMarker* Marker = World->SpawnActor<APDPingMarker>(
                MarkerClass, InWorldLocation, FRotator::ZeroRotator, Params))
        {
            Marker->InitializePing(InType);
            Data.MarkerActor = Marker;
        }
    }

    ActivePings.Add(Data.PingId, Data);
    OnPingAdded.Broadcast(Data);
    return Data.PingId;
}

bool UPDPingSubsystem::RemovePing(int32 InPingId)
{
    FPDPingData Data;
    if (!ActivePings.RemoveAndCopyValue(InPingId, Data))
    {
        return false;
    }

    if (Data.MarkerActor.IsValid())
    {
        Data.MarkerActor->Destroy();
    }
    OnPingRemoved.Broadcast(InPingId);
    return true;
}

void UPDPingSubsystem::ClearAllPings()
{
    TArray<int32> Ids;
    ActivePings.GetKeys(Ids);
    for (int32 Id : Ids)
    {
        RemovePing(Id);
    }
}

void UPDPingSubsystem::GetActivePings(TArray<FPDPingData>& OutPings) const
{
    OutPings.Reset();
    ActivePings.GenerateValueArray(OutPings);
}

void UPDPingSubsystem::TickExpiration()
{
    UWorld* World = GetWorld();
    if (!World) return;

    const float Now = World->GetTimeSeconds();

    TArray<int32> Expired;
    for (const auto& Pair : ActivePings)
    {
        if (Pair.Value.ExpireTime > 0.f && Now >= Pair.Value.ExpireTime)
        {
            Expired.Add(Pair.Key);
        }
    }
    for (int32 Id : Expired)
    {
        //Enemy 핑이 만료되면 잔존 표식 자동 생성
        if (const FPDPingData* Data = ActivePings.Find(Id))
        {
            if (Data->PingType == EPDPingType::Enemy)
            {
                AddFaintMark(Data->WorldLocation);
            }
        }
        RemovePing(Id);
    }
}

//-------------------잔존 표식-------------------
int32 UPDPingSubsystem::AddFaintMark(const FVector& InWorldLocation)
{
    FPDFaintMark NewMark;
    NewMark.FaintId = NextFaintId++;
    NewMark.WorldLocation = InWorldLocation;

    ActiveFaintMarks.Add(NewMark.FaintId, NewMark);
    OnFaintMarkAdded.Broadcast(NewMark);
    return NewMark.FaintId;
}

bool UPDPingSubsystem::RemoveFaintMark(int32 InFaintId)
{
    if (ActiveFaintMarks.Remove(InFaintId) > 0)
    {
        OnFaintMarkRemoved.Broadcast(InFaintId);
        return true;
    }
    return false;
}

bool UPDPingSubsystem::RemoveFaintMarkNearLocation(const FVector& InWorldLocation, float Radius)
{
    int32 BestId = -1;
    float BestDistSq = Radius * Radius;

    for (const TPair<int32, FPDFaintMark>& Pair : ActiveFaintMarks)
    {
        const float DistSq = FVector::DistSquared2D(Pair.Value.WorldLocation, InWorldLocation);
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestId = Pair.Key;
        }
    }

    if (BestId != -1)
    {
        return RemoveFaintMark(BestId);
    }
    return false;
}

void UPDPingSubsystem::ClearAllFaintMarks()
{
    TArray<int32> Ids;
    ActiveFaintMarks.GetKeys(Ids);
    for (int32 Id : Ids)
    {
        RemoveFaintMark(Id);
    }
}

void UPDPingSubsystem::GetActiveFaintMarks(TArray<FPDFaintMark>& OutMarks) const
{
    OutMarks.Reset();
    ActiveFaintMarks.GenerateValueArray(OutMarks);
}

void UPDPingSubsystem::StartEnemyDetectionCheck(float Interval)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (Interval <= 0.f) Interval = 1.0f;

    World->GetTimerManager().SetTimer(
        FaintMarkCheckTimer, this, &UPDPingSubsystem::CheckFaintMarksForEnemy,
        Interval, true);
}

void UPDPingSubsystem::StopEnemyDetectionCheck()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FaintMarkCheckTimer);
    }
}

void UPDPingSubsystem::CheckFaintMarksForEnemy()
{
    if (ActiveFaintMarks.Num() == 0) return;

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return;

    APawn* PlayerPawn = PC->GetPawn();
    if (!PlayerPawn) return;

    //플레이어의 VisionComponent에서 현재 시야 안 액터 목록 가져옴
    UPDVisionComponent* VisionComp = PlayerPawn->FindComponentByClass<UPDVisionComponent>();
    if (!VisionComp) return;

    const TSet<AActor*>& VisibleActors = VisionComp->GetVisibleActors();
    if (VisibleActors.Num() == 0) return;

    const float RadiusSq = FaintMarkDetectRadius * FaintMarkDetectRadius;

    TArray<int32> ToRemove;
    for (const TPair<int32, FPDFaintMark>& Pair : ActiveFaintMarks)
    {
        for (AActor* Enemy : VisibleActors)
        {
            if (!IsValid(Enemy)) continue;

            //적 캐릭터만
            APDEnemyBase* EnemyBase = Cast<APDEnemyBase>(Enemy);
            if (!EnemyBase) continue;
            if (EnemyBase->GetEnemyState() == EPDEnemyState::Dead) continue; //죽은 적

            //적이 잔존 표식 반경 안
            const float DistSq = FVector::DistSquared2D(Pair.Value.WorldLocation, Enemy->GetActorLocation());
            if (DistSq < RadiusSq)
            {
                ToRemove.Add(Pair.Key);
                break;
            }
        }
    }

    for (int32 Id : ToRemove)
    {
        RemoveFaintMark(Id);
    }
}