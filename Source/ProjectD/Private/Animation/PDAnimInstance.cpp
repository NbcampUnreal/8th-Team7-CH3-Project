#include "Animation/PDAnimInstance.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Characters/PDPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapons/PDWeaponBase.h"

void UPDAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwnerCharacter=Cast<APDPlayerCharacter>(GetOwningActor());
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

	if (CachedASC->HasMatchingGameplayTag(PDGameplayTags::Weapon_Type_Rifle))
		Cache.WeaponType=EWeaponType::Rifle;
	else if (CachedASC->HasMatchingGameplayTag(PDGameplayTags::Weapon_Type_Shotgun))
		Cache.WeaponType=EWeaponType::Shotgun;
	else if (CachedASC->HasMatchingGameplayTag(PDGameplayTags::Weapon_Type_Sniper))
		Cache.WeaponType=EWeaponType::Sniper;
	else
		Cache.WeaponType=EWeaponType::None;

	APDWeaponBase* Weapon=OwnerCharacter->GetCurrentWeapon();
	if (!IsValid(Weapon)||!IsValid(Weapon->GetWeaponMesh()))
	{
		Cache.LeftHandIKAlpha=0.f;
		return;
	}

	// const FName GripSocket=Weapon->GetLeftHandGripSocket();
	// if (Weapon->GetWeaponMesh()->DoesSocketExist(GripSocket))
	// {
	// 	Cache.LeftHandIKTarget=Weapon->GetWeaponMesh()->GetSocketLocation(GripSocket);
	// 	Cache.LeftHandIKAlpha=1.f;
	// }
	// else
	// 	Cache.LeftHandIKAlpha=0.f;
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
}