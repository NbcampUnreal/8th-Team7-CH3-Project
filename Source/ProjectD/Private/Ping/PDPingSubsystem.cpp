#include "Ping/PDPingSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Ping/PDPingMarker.h"

void UPDPingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// 만약 BP_PD_PingMarker 경로가 바뀌면 아래 경로도 같이 수정
	if (DefaultMarkerClass.IsNull())
	{
		DefaultMarkerClass = TSoftClassPtr<APDPingMarker>(
			FSoftObjectPath(TEXT("/Game/Developers/dbals/Ping/BP_PD_PingMarker.BP_PD_PingMarker_C"))
		);
	}

	if (UWorld* World = GetWorld())
	{
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
	}
	ClearAllPings();
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

	FPDPingData Data;
	Data.PingId = NextPingId++;
	Data.PingType = InType;
	Data.WorldLocation = InWorldLocation;
	Data.ExpireTime = (DefaultPingLifetime > 0.f) ? World->GetTimeSeconds() + DefaultPingLifetime : 0.f;

	if (UClass* MarkerClass = DefaultMarkerClass.LoadSynchronous())
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
		RemovePing(Id);
	}
}