#include "Animation/PDAnimInstance.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Characters/Base/PDCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "Weapons/Base/PDRangedWeaponBase.h"

void UPDAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwnerCharacter=Cast<APDCharacterBase>(GetOwningActor());
	if (IsValid(OwnerCharacter))
		CachedASC=UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerCharacter);
}

void UPDAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!IsValid(OwnerCharacter)||!IsValid(CachedASC)) return;

	const FVector Velocity=OwnerCharacter->GetCharacterMovement()->Velocity;
	Cache.MovementSpeed=Velocity.Size2D();

	Cache.bIsJumping=OwnerCharacter->GetCharacterMovement()->IsFalling();
	Cache.Direction=UKismetMathLibrary::NormalizedDeltaRotator(
		UKismetMathLibrary::MakeRotFromX(Velocity),
		OwnerCharacter->GetActorRotation()).Yaw;
	
	Cache.bIsAiming=CachedASC->HasMatchingGameplayTag(PDGameplayTags::State_Aiming);
	Cache.AimYaw=0.f;
	Cache.AimPitch=0.f;
	Cache.bIsInCover=CachedASC->HasMatchingGameplayTag(PDGameplayTags::Cover_Active);
	Cache.bIsCoverAiming=CachedASC->HasMatchingGameplayTag(PDGameplayTags::State_CoverAim);
	Cache.bIsMeleeEquipped=CachedASC->HasMatchingGameplayTag(PDGameplayTags::Weapon_Type_Melee);


	if (CachedASC->HasMatchingGameplayTag(PDGameplayTags::Weapon_Type_Rifle))
		Cache.WeaponType=EWeaponType::Rifle;
	else if (CachedASC->HasMatchingGameplayTag(PDGameplayTags::Weapon_Type_Shotgun))
		Cache.WeaponType=EWeaponType::Shotgun;
	else if (CachedASC->HasMatchingGameplayTag(PDGameplayTags::Weapon_Type_Sniper))
		Cache.WeaponType=EWeaponType::Sniper;
	else if (CachedASC->HasMatchingGameplayTag(PDGameplayTags::Weapon_Type_Melee))
		Cache.WeaponType=EWeaponType::Melee;
	else
		Cache.WeaponType=EWeaponType::None;

	APDWeaponBase* Weapon=OwnerCharacter->GetCurrentWeapon();
	if (!IsValid(Weapon)||!IsValid(Weapon->GetWeaponMesh()))
	{
		Cache.LeftHandIKAlpha=0.f;
		return;
	}

	const FName GripSocket=Weapon->GetLeftHandGripSocket();
	if (Weapon->GetWeaponMesh()->DoesSocketExist(GripSocket))
	{
		Cache.LeftHandIKTarget=Weapon->GetWeaponMesh()->GetSocketLocation(GripSocket);
		Cache.LeftHandIKAlpha=1.f;
	}
	else
		Cache.LeftHandIKAlpha=0.f;
}

void UPDAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	MovementSpeed=Cache.MovementSpeed;
	bIsJumping=Cache.bIsJumping;
	Direction=Cache.Direction;
	bIsAiming=Cache.bIsAiming;
	AimYaw=Cache.AimYaw;
	AimPitch=Cache.AimPitch;
	WeaponType=Cache.WeaponType;
	LeftHandIKTarget=Cache.LeftHandIKTarget;
	LeftHandIKAlpha=Cache.LeftHandIKAlpha;
	bIsInCover=Cache.bIsInCover;
	bIsCoverAiming=Cache.bIsCoverAiming;
	bIsMeleeEquipped=Cache.bIsMeleeEquipped;
}

// ── 무기 이벤트 바인딩 ──────────────────────────────────────────────────────

void UPDAnimInstance::OnWeaponEquipped(APDRangedWeaponBase* Weapon)
{
	OnWeaponUnequipped(BoundWeapon.Get());   // 이전 무기 바인딩 해제

	if (!Weapon) return;
	BoundWeapon = Weapon;

	Weapon->OnWeaponFired.AddDynamic(this, &UPDAnimInstance::HandleWeaponFired);
	Weapon->OnWeaponReloadStarted.AddDynamic(this, &UPDAnimInstance::HandleWeaponReloadStarted);

	// Equip 중 발사 차단
	Weapon->bCanFire = false;

	const FPDWeaponAnimSet* Set = GetAnimSetForWeapon(Weapon);
	if (Set && Set->EquipMontage)
	{
		Montage_Play(Set->EquipMontage);
		if (Set->EquipStartSection != NAME_None)
			Montage_JumpToSection(Set->EquipStartSection, Set->EquipMontage);

		// 몽타주 종료 시 발사 허용
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UPDAnimInstance::OnEquipMontageEnded);
		Montage_SetEndDelegate(EndDelegate, Set->EquipMontage);
	}
	else
	{
		// 몽타주 없으면 즉시 해제
		Weapon->bCanFire = true;
	}
}

void UPDAnimInstance::OnEquipMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (BoundWeapon.IsValid())
		BoundWeapon->bCanFire = true;
}

void UPDAnimInstance::OnWeaponUnequipped(APDRangedWeaponBase* Weapon)
{
	if (!Weapon || BoundWeapon.Get() != Weapon) return;

	Weapon->OnWeaponFired.RemoveDynamic(this, &UPDAnimInstance::HandleWeaponFired);
	Weapon->OnWeaponReloadStarted.RemoveDynamic(this, &UPDAnimInstance::HandleWeaponReloadStarted);

	BoundWeapon = nullptr;
}

void UPDAnimInstance::HandleWeaponFired(APDWeaponBase* Weapon)
{
	if (const FPDWeaponAnimSet* Set = GetAnimSetForWeapon(Weapon))
		if (Set->FireMontage)
			Montage_Play(Set->FireMontage);
}

void UPDAnimInstance::HandleWeaponReloadStarted(APDWeaponBase* Weapon)
{
	if (const FPDWeaponAnimSet* Set = GetAnimSetForWeapon(Weapon))
	{
		if (Set->ReloadMontage)
		{
			Montage_Play(Set->ReloadMontage);
			if (Set->ReloadStartSection != NAME_None)
				Montage_JumpToSection(Set->ReloadStartSection, Set->ReloadMontage);
		}
	}
}

const FPDWeaponAnimSet* UPDAnimInstance::GetAnimSetForWeapon(APDWeaponBase* Weapon) const
{
	if (!Weapon) return nullptr;

	const FGameplayTag& Tag = Weapon->GetWeaponTypeTag();

	if (Tag == PDGameplayTags::Weapon_Type_Rifle)   return &RifleAnimSet;
	if (Tag == PDGameplayTags::Weapon_Type_Shotgun) return &ShotgunAnimSet;
	if (Tag == PDGameplayTags::Weapon_Type_Sniper)  return &SniperAnimSet;
	if (Tag == PDGameplayTags::Weapon_Type_Melee)   return &MeleeAnimSet;

	return nullptr;
}

// ── 피격 반응 ────────────────────────────────────────────────────────────────

void UPDAnimInstance::PlayHitReaction()
{
	TArray<UAnimMontage*, TInlineAllocator<4>> Available;
	if (HitMontage_1) Available.Add(HitMontage_1);
	if (HitMontage_2) Available.Add(HitMontage_2);
	if (HitMontage_3) Available.Add(HitMontage_3);
	if (HitMontage_4) Available.Add(HitMontage_4);

	if (Available.IsEmpty()) return;

	Montage_Play(Available[FMath::RandRange(0, Available.Num() - 1)]);
}