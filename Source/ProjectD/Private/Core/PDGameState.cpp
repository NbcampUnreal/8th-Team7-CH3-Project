#include "Core/PDGameState.h"

void APDGameState::SetRaidState(ERaidState NewState)
{
	if (CurrentRaidState==NewState) return;
	CurrentRaidState=NewState;
	OnRaidStateChanged(NewState);
}