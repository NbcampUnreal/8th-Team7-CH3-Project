#include "Enemy/Characters/PDScavenger.h"

#include "Engine/World.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "Items/PDStashActor.h"
#include "Items/PDStashComponent.h"
#include "Weapons/Base/PDWeaponBase.h"

APDScavenger::APDScavenger()
{
	TeamID = 2; // Hostile.
}

void APDScavenger::BeginPlay()
{
	Super::BeginPlay();

	SpawnAndEquipDefaultWeapon();

	if (UPDCombatComponent* Combat = GetCombatComponent())
	{
		Combat->OnAttackRequested.AddDynamic(this, &APDScavenger::HandleAttackRequested);
	}
}

void APDScavenger::OnEnterState_Dead()
{
	Super::OnEnterState_Dead();

	if (!EquippedWeapon) return;

	EquippedWeapon->OnUnequip();

	// 베이스가 스폰한 시체 컨테이너가 Stash 류면 무기 데이터를 그 안으로 이전. 픽업은 LootBox 상호작용으로만.
	bool bTransferred = false;
	if (APDStashActor* Corpse = Cast<APDStashActor>(GetCorpseContainer()))
	{
		if (UPDStashComponent* Stash = Corpse->GetStashComponent())
		{
			const FName WeaponItemID = EquippedWeapon->GetItemID();
			if (!WeaponItemID.IsNone() && Stash->AddItemByID(WeaponItemID, 1))
			{
				bTransferred = true;
			}
		}
	}

	if (!bTransferred)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] CorpseContainer 가 Stash 류가 아니거나 WeaponData 미지정 — 장착무기 소실."),
			*GetName());
	}

	EquippedWeapon->Destroy();
	EquippedWeapon = nullptr;
}

void APDScavenger::SpawnAndEquipDefaultWeapon()
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

void APDScavenger::SetEquippedWeapon(APDWeaponBase* NewWeapon, bool bDestroyPrevious)
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

void APDScavenger::HandleAttackRequested(AActor* /*Target*/)
{
	if (!bAutoFireOnAttackRequested) return;

	if (!EquippedWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] 공격 요청 — EquippedWeapon 없음."), *GetName());
		return;
	}

	EquippedWeapon->Fire();
}
