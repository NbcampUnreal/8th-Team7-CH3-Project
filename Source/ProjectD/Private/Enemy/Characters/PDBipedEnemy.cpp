#include "Enemy/Characters/PDBipedEnemy.h"

#include "Enemy/Components/PDStatComponent.h"
#include "Enemy/Components/PDCombatComponent.h"

APDBipedEnemy::APDBipedEnemy()
{
	StatComponent   = CreateDefaultSubobject<UPDStatComponent>  (TEXT("StatComponent"));
	CombatComponent = CreateDefaultSubobject<UPDCombatComponent>(TEXT("CombatComponent"));

	if (StatComponent)
	{
		StatComponent->SetUseStamina(true);
	}
}

EPDStaminaStatus APDBipedEnemy::GetStaminaStatus_Implementation() const
{
	if (StatComponent && StatComponent->IsUsingStamina())
	{
		return StatComponent->GetStaminaStatus();
	}
	return EPDStaminaStatus::None;
}
