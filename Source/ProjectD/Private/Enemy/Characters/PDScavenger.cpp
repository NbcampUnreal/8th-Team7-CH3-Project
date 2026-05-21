#include "Enemy/Characters/PDScavenger.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/PDAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "Weapons/Base/PDRangedWeaponBase.h"

APDScavenger::APDScavenger()
{
	TeamID = 2; // Hostile.
}

void APDScavenger::BeginPlay()
{
	Super::BeginPlay();

	// 무기 타입 태그 변경 시 AnimLayer 변경
	if (ASC)
	{
		ASC->RegisterGameplayTagEvent(PDGameplayTags::Weapon_Type_Melee,   EGameplayTagEventType::NewOrRemoved).AddUObject(this, &APDScavenger::OnWeaponTypeTagChanged);
	}
	LinkDefaultAnimLayer();

	SpawnAndEquipDefaultWeapon();

	if (UPDCombatComponent* Combat = GetCombatComponent())
	{
		Combat->OnAttackRequested.AddDynamic(this, &APDScavenger::HandleAttackRequested);
	}
}

void APDScavenger::OnWeaponTypeTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount == 0) return;

	APDWeaponBase* CurWeapon = GetCurrentWeapon();
	if (!IsValid(CurWeapon)) return;

	TSubclassOf<UAnimInstance> LayerClass = CurWeapon->GetWeaponAnimLayerClass();
	if (!LayerClass) { LinkDefaultAnimLayer(); return; }

	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->LinkAnimClassLayers(LayerClass);
	}
}

void APDScavenger::LinkDefaultAnimLayer()
{
	if (!DefaultAnimLayerClass) return;
	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->LinkAnimClassLayers(DefaultAnimLayerClass);
	}
}

void APDScavenger::OnEnterState_Dead()
{
	// 무기 → 시체 Stash 이전은 EnemyBase::OnEnterState_Dead 안의 TryDropEquippedWeaponToCorpse 가
	// WeaponDropChance 확률로 처리. 본 클래스는 무기 액터 정리만 담당.
	Super::OnEnterState_Dead();

	if (!EquippedWeapon) return;

	EquippedWeapon->OnUnequip();
	EquippedWeapon->Destroy();
	EquippedWeapon = nullptr;
}

FName APDScavenger::GetEquippedWeaponItemID_Implementation() const
{
	return EquippedWeapon ? EquippedWeapon->GetItemID() : NAME_None;
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

	UPDAnimInstance* AnimInst = GetMesh() ? Cast<UPDAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr;

	if (EquippedWeapon)
	{
		// 무기 타입 태그 제거 — AnimInstance.WeaponType 캐시가 None 으로 복귀.
		if (ASC)
		{
			ASC->RemoveLooseGameplayTag(EquippedWeapon->GetWeaponTypeTag());
		}
		if (AnimInst)
		{
			AnimInst->OnWeaponUnequipped(Cast<APDRangedWeaponBase>(EquippedWeapon));
		}

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

		// AnimInstance 가 ASC 태그를 보고 WeaponType/bIsMelee 를 결정.
		if (ASC)
		{
			ASC->AddLooseGameplayTag(EquippedWeapon->GetWeaponTypeTag());
		}
	}
}

void APDScavenger::HandleAttackRequested(AActor* /*Target*/)
{
	if (!bAutoFireOnAttackRequested) return;

	// 상태 게이트 — BT 가 Combat 분기에서 SetEnemyState(Combat) 호출했을 때만 휘두름.
	// Idle/Alert/Chase 단계에서 우발적 RequestAttack 이 와도 무시 (Soldier 의 OnFireTick 게이트와 동일 정책).
	if (GetEnemyState() != EPDEnemyState::Combat) return;

	if (!EquippedWeapon) return;
	if (!ASC || !MeleeAttackAbilityClass) return;

	// Ability 활성화 — 몽타주 재생/Anim Notify 대기/sweep/데미지 인가는 GA 내부에서 처리.
	// 디자이너 노트: BP_PDScavenger 의 ActiveAbilities 배열에 같은 클래스가 등록돼 있어야 GiveAbility 됨.
	ASC->TryActivateAbilityByClass(MeleeAttackAbilityClass);
}
