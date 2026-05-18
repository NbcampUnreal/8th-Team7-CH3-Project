#include "Enemy/Characters/PDScavenger.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/PDAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "Enemy/Interfaces/PDCombatInterface.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Interfaces/PDDamageable.h"
#include "Items/PDStashActor.h"
#include "Items/PDStashComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Weapons/Base/PDMeleeWeaponBase.h"
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

	if (!EquippedWeapon) return;

	// 휘두름 몽타주 재생 — 시각적 모션. sweep 와 별개 트랙.
	if (AttackMontage && !AttackSections.IsEmpty())
	{
		if (UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			const FName Section = AttackSections[FMath::RandRange(0, AttackSections.Num() - 1)];
			AnimInst->Montage_Play(AttackMontage);
			AnimInst->Montage_JumpToSection(Section, AttackMontage);
		}
	}

	PerformMeleeAttack();
}

void APDScavenger::PerformMeleeAttack()
{
	APDMeleeWeaponBase* MeleeWeapon = Cast<APDMeleeWeaponBase>(EquippedWeapon);
	if (!MeleeWeapon) return;

	UWorld* World = GetWorld();
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!World || !MeshComp) return;

	// Swing 사운드 (휘두름 시작).
	if (USoundBase* Sound = MeleeWeapon->GetSwingSound())
	{
		UGameplayStatics::SpawnSoundAttached(Sound, MeshComp);
	}

	const float Damage      = MeleeWeapon->GetCurrentStats().Damage;
	const float SweepRadius = MeleeWeapon->GetSweepRadius();
	const float SweepRange  = MeleeWeapon->GetSweepRange();
	const FName HitSocket   = MeleeWeapon->GetHitSocketName();

	const FVector Start = MeshComp->GetSocketLocation(HitSocket);
	const FVector End   = Start + GetActorForwardVector() * SweepRange;

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PD_ScavengerMeleeSweep), false, this);
	World->SweepMultiByChannel(
		Hits, Start, End, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeSphere(SweepRadius), Params);

	const uint8 MyTeam = TeamID;
	TSet<AActor*> HitOnce;
	bool bPlayedHitSound = false;

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!IsValid(HitActor) || HitOnce.Contains(HitActor)) continue;
		if (!HitActor->Implements<UPDDamageable>()) continue;

		// Friendly fire 차단 — 같은 팀이면 sweep 명중해도 무시.
		if (HitActor->Implements<UPDCombatInterface>())
		{
			if (IPDCombatInterface::Execute_GetTeamID(HitActor) == MyTeam) continue;
		}

		HitOnce.Add(HitActor);

		if (!bPlayedHitSound)
		{
			if (USoundBase* HitSnd = MeleeWeapon->GetHitSound())
			{
				UGameplayStatics::PlaySoundAtLocation(World, HitSnd, Hit.ImpactPoint);
			}
			bPlayedHitSound = true;
		}

		FPDDamageInfo Info;
		Info.BaseDamage     = Damage;
		Info.Instigator     = this;
		Info.HitResult      = Hit;
		IPDDamageable::Execute_ApplyDamage(HitActor, Info);
	}
}
