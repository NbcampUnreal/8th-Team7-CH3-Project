#include "Characters/Base/PDEnemyBase.h"
#include "AbilitySystemComponent.h"

APDEnemyBase::APDEnemyBase()
{
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
}

void APDEnemyBase::SetEnemyState(EPDEnemyState NewState)
{
	if (CurrentState == NewState) return;

	CurrentState = NewState;
	OnEnemyStateChanged(NewState);
}

void APDEnemyBase::OnVisionExposureChanged_Implementation(AActor* Observer, float Exposure)
{
}

void APDEnemyBase::HandleDeath(AActor* Killer)
{
	SetEnemyState(EPDEnemyState::Dead);
	Super::HandleDeath(Killer);
}
