#include "Enemy/Components/PDStatComponent.h"

UPDStatComponent::UPDStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDStatComponent::BeginPlay()
{
	Super::BeginPlay();

	FScopeLock Lock(&StaminaCS);
	MaxStamina = FMath::Max(0.f, MaxStamina);
	CurrentStamina = FMath::Clamp(InitialStamina, 0.f, MaxStamina);
	CachedStaminaStatus = bUseStamina ? ComputeStaminaStatus(GetStaminaPercent()) : EPDStaminaStatus::None;
}

void UPDStatComponent::SetUseStamina(bool bInUseStamina)
{
	if (bUseStamina == bInUseStamina) return;

	bUseStamina = bInUseStamina;

	const EPDStaminaStatus NewStatus = bUseStamina ? ComputeStaminaStatus(GetStaminaPercent()) : EPDStaminaStatus::None;
	const EPDStaminaStatus OldStatus = CachedStaminaStatus;
	if (OldStatus != NewStatus)
	{
		CachedStaminaStatus = NewStatus;
		OnStaminaStatusChanged.Broadcast(OldStatus, NewStatus);
	}
}

float UPDStatComponent::GetCurrentStamina() const
{
	if (!bUseStamina) return 0.f;
	FScopeLock Lock(&StaminaCS);
	return CurrentStamina;
}

float UPDStatComponent::GetMaxStamina() const
{
	FScopeLock Lock(&StaminaCS);
	return MaxStamina;
}

float UPDStatComponent::GetStaminaPercent() const
{
	FScopeLock Lock(&StaminaCS);
	if (!bUseStamina || MaxStamina <= 0.f) return 0.f;
	return CurrentStamina / MaxStamina;
}

EPDStaminaStatus UPDStatComponent::GetStaminaStatus() const
{
	if (!bUseStamina) return EPDStaminaStatus::None;
	FScopeLock Lock(&StaminaCS);
	return CachedStaminaStatus;
}

void UPDStatComponent::SetStamina(float NewValue)
{
	if (!bUseStamina) return;
	ApplyStaminaChange_Internal(NewValue);
}

void UPDStatComponent::SetMaxStamina(float NewMax)
{
	if (NewMax < 0.f) NewMax = 0.f;

	float ClampedCurrent = 0.f;
	{
		FScopeLock Lock(&StaminaCS);
		MaxStamina = NewMax;
		ClampedCurrent = FMath::Clamp(CurrentStamina, 0.f, MaxStamina);
	}
	ApplyStaminaChange_Internal(ClampedCurrent);
}

bool UPDStatComponent::ConsumeStamina(float Amount)
{
	if (!bUseStamina || Amount <= 0.f) return false;

	float NewValue = 0.f;
	bool bFullySatisfied = false;
	{
		FScopeLock Lock(&StaminaCS);
		bFullySatisfied = (CurrentStamina >= Amount);
		NewValue = FMath::Max(0.f, CurrentStamina - Amount);
	}
	ApplyStaminaChange_Internal(NewValue);
	return bFullySatisfied;
}

void UPDStatComponent::RecoverStamina(float Amount)
{
	if (!bUseStamina || Amount <= 0.f) return;

	float NewValue = 0.f;
	{
		FScopeLock Lock(&StaminaCS);
		NewValue = FMath::Clamp(CurrentStamina + Amount, 0.f, MaxStamina);
	}
	ApplyStaminaChange_Internal(NewValue);
}

EPDStaminaStatus UPDStatComponent::ComputeStaminaStatus(float Percent) const
{
	if (Percent >= FullThreshold)    return EPDStaminaStatus::Full;
	if (Percent >= OptimalThreshold) return EPDStaminaStatus::Optimal;
	if (Percent >= LowThreshold)     return EPDStaminaStatus::Low;
	return EPDStaminaStatus::Exhausted;
}

void UPDStatComponent::ApplyStaminaChange_Internal(float NewValue)
{
	float Old = 0.f;
	float NewClamped = 0.f;
	float MaxRef = 0.f;
	EPDStaminaStatus OldStatus = EPDStaminaStatus::None;
	EPDStaminaStatus NewStatus = EPDStaminaStatus::None;
	bool bDepletedNow = false;

	{
		FScopeLock Lock(&StaminaCS);
		MaxRef = MaxStamina;
		Old = CurrentStamina;
		NewClamped = FMath::Clamp(NewValue, 0.f, MaxRef);

		if (FMath::IsNearlyEqual(Old, NewClamped))
		{
			return;
		}

		CurrentStamina = NewClamped;

		OldStatus = CachedStaminaStatus;
		const float Percent = (MaxRef > 0.f) ? (NewClamped / MaxRef) : 0.f;
		NewStatus = bUseStamina ? ComputeStaminaStatus(Percent) : EPDStaminaStatus::None;
		CachedStaminaStatus = NewStatus;

		bDepletedNow = (NewClamped <= 0.f && Old > 0.f);
	}

	// 락 밖에서 broadcast — 구독자가 본 컴포넌트를 다시 호출해도 데드락 없도록.
	OnStaminaChanged.Broadcast(NewClamped, MaxRef);

	if (OldStatus != NewStatus)
	{
		OnStaminaStatusChanged.Broadcast(OldStatus, NewStatus);
	}

	if (bDepletedNow)
	{
		OnStaminaDepleted.Broadcast();
	}
}
