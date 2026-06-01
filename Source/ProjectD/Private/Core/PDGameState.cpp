#include "Core/PDGameState.h"
#include "Net/UnrealNetwork.h"

void APDGameState::SetRaidState(ERaidState NewState)
{
	if (CurrentRaidState==NewState) return;
	CurrentRaidState=NewState;
	OnRaidStateChanged(NewState);
}

void APDGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APDGameState, ZoneTravelType);
	DOREPLIFETIME(APDGameState, ZonePlayersInZone);
	DOREPLIFETIME(APDGameState, ZoneTotalParticipants);
	DOREPLIFETIME(APDGameState, ZoneCountdownEndServerTime);
	DOREPLIFETIME(APDGameState, bZoneFinalCountdown);
}

void APDGameState::SetZoneCountdown(EPDZoneTravelType InType, int32 InPlayersInZone, int32 InTotal, float InEndServerTime, bool bInFinal)
{
	if (!HasAuthority()) return;

	ZoneTravelType = InType;
	ZonePlayersInZone = InPlayersInZone;
	ZoneTotalParticipants = InTotal;
	ZoneCountdownEndServerTime = InEndServerTime;
	bZoneFinalCountdown = bInFinal;

	ForceNetUpdate();
	OnZoneCountdownChanged.Broadcast();
}

void APDGameState::ClearZoneCountdown()
{
	if (!HasAuthority()) return;

	ZoneTravelType = EPDZoneTravelType::None;
	ZonePlayersInZone = 0;
	ZoneTotalParticipants = 0;
	ZoneCountdownEndServerTime = -1.f;
	bZoneFinalCountdown = false;

	ForceNetUpdate();
	OnZoneCountdownChanged.Broadcast();
}

float APDGameState::GetZoneCountdownRemaining() const
{
	if (ZoneTravelType == EPDZoneTravelType::None || ZoneCountdownEndServerTime < 0.f)
	{
		return 0.f;
	}

	return FMath::Max(0.f, ZoneCountdownEndServerTime - GetServerWorldTimeSeconds());
}

void APDGameState::OnRep_ZoneCountdown()
{
	OnZoneCountdownChanged.Broadcast();
}