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
	// Player/Enemy 공통 베이스로 캐스팅: AnimBP를 양쪽이 공유 가능
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
	Cache.bIsDowned=CachedASC->HasMatchingGameplayTag(PDGameplayTags::State_Downed);
	Cache.bIsSprinting=CachedASC->HasMatchingGameplayTag(PDGameplayTags::State_Sprinting);

	APDWeaponBase* Weapon=OwnerCharacter->GetCurrentWeapon();
	// 베이스 캐릭터 기반이므로 무기 자체에서 타입을 읽어 player/enemy 공통 처리
	Cache.WeaponType=Weapon ? Weapon->GetWeaponType() : EWeaponType::None;
	Cache.bIsMeleeEquipped=Cache.WeaponType == EWeaponType::Melee;

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
	bIsMeleeEquipped=Cache.bIsMeleeEquipped;
	bIsDowned=Cache.bIsDowned;
	bIsSprinting=Cache.bIsSprinting;
}



void UPDAnimInstance::OnWeaponEquipped(APDRangedWeaponBase* Weapon)
{
	OnWeaponUnequipped(BoundWeapon.Get());

	if (!Weapon) return;
	BoundWeapon = Weapon;

	Weapon->OnWeaponFired.AddDynamic(this, &UPDAnimInstance::HandleWeaponFired);
	Weapon->OnWeaponReloadStarted.AddDynamic(this, &UPDAnimInstance::HandleWeaponReloadStarted);


	Weapon->bCanFire = false;

	const FPDWeaponAnimSet* Set = GetAnimSetForWeapon(Weapon);
	if (Set && Set->EquipMontage)
	{
		Montage_Play(Set->EquipMontage);
		if (Set->EquipStartSection != NAME_None)
			Montage_JumpToSection(Set->EquipStartSection, Set->EquipMontage);


		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UPDAnimInstance::OnEquipMontageEnded);
		Montage_SetEndDelegate(EndDelegate, Set->EquipMontage);
	}
	else
	{

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

float UPDAnimInstance::GetReloadMontageDurationForWeapon(APDWeaponBase* Weapon) const
{
	if (const FPDWeaponAnimSet* Set = GetAnimSetForWeapon(Weapon))
	{
		return Set->ReloadMontage ? Set->ReloadMontage->GetPlayLength() : 0.f;
	}
	return 0.f;
}

void UPDAnimInstance::StopReloadMontageForWeapon(APDWeaponBase* Weapon, float BlendOutTime)
{
	if (const FPDWeaponAnimSet* Set = GetAnimSetForWeapon(Weapon))
	{
		if (Set->ReloadMontage)
		{
			Montage_Stop(BlendOutTime, Set->ReloadMontage);
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

void UPDAnimInstance::PlayGetUpMontage()
{
	if (!GetUpMontage) return;

	Montage_Play(GetUpMontage);
}

float UPDAnimInstance::GetGetUpMontageDuration() const
{
	if (GetUpDuration > KINDA_SMALL_NUMBER)
	{
		return GetUpDuration;
	}

	return GetUpMontage ? GetUpMontage->GetPlayLength() : 0.f;
}
