#include "Weapons/PDWeaponBase.h"

APDWeaponBase::APDWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APDWeaponBase::Fire_Implementation()
{
}

void APDWeaponBase::OnEquip_Implementation(AActor* NewOwner)
{
	WeaponOwner = NewOwner;
}

void APDWeaponBase::OnUnequip_Implementation()
{
	WeaponOwner = nullptr;
}
