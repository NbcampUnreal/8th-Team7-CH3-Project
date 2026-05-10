#include "Enemy/Characters/PDSoldier.h"

#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "Weapons/PDWeaponBase.h"

APDSoldier::APDSoldier()
{
	TeamID = 2; // Hostile.
}

void APDSoldier::BeginPlay()
{
	Super::BeginPlay();

	SpawnAndEquipDefaultWeapon();

	// CombatComponent.OnAttackRequested 는 "공격 의도" 신호 — 무기에게 Fire 위임.
	if (UPDCombatComponent* Combat = GetCombatComponent())
	{
		Combat->OnAttackRequested.AddDynamic(this, &APDSoldier::HandleAttackRequested);
	}
}

void APDSoldier::OnEnterState_Dead()
{
	Super::OnEnterState_Dead();

	// 사망 시 무기 떨굼 — Drop 처리는 무기 측 IsDropped 플래그가 있어 BP 디자이너가 후속 정책 결정.
	if (EquippedWeapon)
	{
		EquippedWeapon->OnUnequip();
		EquippedWeapon->SetDropped(true);
	}
}

void APDSoldier::SpawnAndEquipDefaultWeapon()
{
	if (!DefaultWeaponClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APDWeaponBase* NewWeapon = World->SpawnActor<APDWeaponBase>(
		DefaultWeaponClass,
		GetActorLocation(),
		GetActorRotation(),
		SpawnParams);

	if (!NewWeapon) return;

	SetEquippedWeapon(NewWeapon, /*bDestroyPrevious=*/true);
}

void APDSoldier::SetEquippedWeapon(APDWeaponBase* NewWeapon, bool bDestroyPrevious)
{
	if (EquippedWeapon == NewWeapon) return;

	if (EquippedWeapon)
	{
		EquippedWeapon->OnUnequip();
		if (bDestroyPrevious)
		{
			EquippedWeapon->Destroy();
		}
	}

	EquippedWeapon = NewWeapon;

	if (EquippedWeapon)
	{
		AttachActorToWeaponSocket(EquippedWeapon);
		EquippedWeapon->OnEquip(this);
	}
}

void APDSoldier::HandleAttackRequested(AActor* /*Target*/)
{
	if (!bAutoFireOnAttackRequested) return;
	if (!EquippedWeapon) return;

	if (AttackMontage)
	{
		PlayAnimMontage(AttackMontage);
	}

	EquippedWeapon->Fire();
}
