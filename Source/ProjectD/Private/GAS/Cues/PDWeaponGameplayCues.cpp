#include "GAS/Cues/PDWeaponGameplayCues.h"

#include "Characters/PDPlayerCharacter.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Animation/PDFootstepDataAsset.h"
#include "Animation/PDAnimInstance.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Weapons/Base/PDMeleeWeaponBase.h"
#include "GameplayTag/PDGameplayTags.h"

#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Sound/SoundBase.h"




namespace
{
	APDPlayerCharacter* GetPDChar(AActor* Target)
	{
		return Cast<APDPlayerCharacter>(Target);
	}

	APDRangedWeaponBase* GetRangedWeapon(AActor* Target)
	{
		APDPlayerCharacter* Char = GetPDChar(Target);
		return Char ? Cast<APDRangedWeaponBase>(Char->GetCurrentWeapon()) : nullptr;
	}

	APDRangedWeaponBase* GetRangedWeapon(AActor* Target, const FGameplayCueParameters& Parameters)
	{
		if (APDRangedWeaponBase* Weapon = Cast<APDRangedWeaponBase>(Parameters.EffectCauser.Get()))
		{
			return Weapon;
		}
		if (APDRangedWeaponBase* Weapon = const_cast<APDRangedWeaponBase*>(Cast<APDRangedWeaponBase>(Parameters.SourceObject.Get())))
		{
			return Weapon;
		}
		return GetRangedWeapon(Target);
	}

	APDMeleeWeaponBase* GetMeleeWeapon(AActor* Target)
	{
		APDPlayerCharacter* Char = GetPDChar(Target);
		return Char ? Cast<APDMeleeWeaponBase>(Char->GetCurrentWeapon()) : nullptr;
	}

	APDMeleeWeaponBase* GetMeleeWeapon(AActor* Target, const FGameplayCueParameters& Parameters)
	{
		if (APDMeleeWeaponBase* Weapon = Cast<APDMeleeWeaponBase>(Parameters.EffectCauser.Get()))
		{
			return Weapon;
		}
		if (APDMeleeWeaponBase* Weapon = const_cast<APDMeleeWeaponBase*>(Cast<APDMeleeWeaponBase>(Parameters.SourceObject.Get())))
		{
			return Weapon;
		}
		return GetMeleeWeapon(Target);
	}

	APDWeaponBase* GetWeapon(AActor* Target, const FGameplayCueParameters& Parameters)
	{
		if (APDWeaponBase* Weapon = Cast<APDWeaponBase>(Parameters.EffectCauser.Get()))
		{
			return Weapon;
		}
		if (APDWeaponBase* Weapon = const_cast<APDWeaponBase*>(Cast<APDWeaponBase>(Parameters.SourceObject.Get())))
		{
			return Weapon;
		}

		APDPlayerCharacter* Char = GetPDChar(Target);
		return Char ? Char->GetCurrentWeapon() : nullptr;
	}
}



UGCN_Weapon_Fire::UGCN_Weapon_Fire()
{
	GameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Weapon.Fire"));
}

bool UGCN_Weapon_Fire::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	APDRangedWeaponBase* Weapon = GetRangedWeapon(MyTarget, Parameters);
	if (!Weapon)
	{
		return false;
	}

	USkeletalMeshComponent* Mesh = Weapon->GetWeaponMesh();


	if (Weapon->MuzzleFlashEffect && Mesh)
	{
		UGameplayStatics::SpawnEmitterAttached(
			Weapon->MuzzleFlashEffect,
			Mesh,
			Weapon->MuzzleSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true);
	}


	if (Weapon->FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(MyTarget, Weapon->FireSound, Parameters.Location);
	}


	if (Parameters.RawMagnitude > 0.f)
	{
		if (Weapon->TracerParticle)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				MyTarget,
				Weapon->TracerParticle,
				Parameters.Location,
				Parameters.Normal.Rotation(),
				true);
		}
	}

	// Cartridge — TODO: 탄피 파티클 에셋 추가 시 여기서 처리


	return true;
}

UGCN_Character_Roll::UGCN_Character_Roll()
{
	GameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Character.Roll"));
}

bool UGCN_Character_Roll::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!MyTarget) return false;

	USoundBase* RollSound = const_cast<USoundBase*>(Cast<USoundBase>(Parameters.SourceObject.Get()));
	if (!RollSound) return false;

	USceneComponent* AttachComponent = Parameters.TargetAttachComponent.Get();
	if (!AttachComponent)
	{
		if (const ACharacter* Character = Cast<ACharacter>(MyTarget))
		{
			AttachComponent = Character->GetMesh();
		}
	}
	if (!AttachComponent)
	{
		AttachComponent = MyTarget->GetRootComponent();
	}

	UGameplayStatics::SpawnSoundAttached(RollSound, AttachComponent);
	return true;
}

UGCN_Character_Footstep::UGCN_Character_Footstep()
{
	GameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Character.Footstep"));
}

bool UGCN_Character_Footstep::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	const UPDFootstepDataAsset* FootstepData = Cast<UPDFootstepDataAsset>(Parameters.SourceObject.Get());
	if (!FootstepData) return false;

	const EPhysicalSurface Surface = static_cast<EPhysicalSurface>(FMath::RoundToInt(Parameters.RawMagnitude));
	const FPDFootstepEntry& Entry = FootstepData->GetEntryForSurface(Surface);
	if (!Entry.Sound && !Entry.StepVFX) return false;

	if (Entry.Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(MyTarget, Entry.Sound, Parameters.Location);
	}

	if (Entry.StepVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(MyTarget, Entry.StepVFX, Parameters.Location);
	}

	return true;
}

UGCN_Weapon_CartridgeHit::UGCN_Weapon_CartridgeHit()
{
	GameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Weapon.CartridgeHit"));
}

bool UGCN_Weapon_CartridgeHit::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	USoundBase* FallSound = const_cast<USoundBase*>(Cast<USoundBase>(Parameters.SourceObject.Get()));
	if (!FallSound) return false;

	UGameplayStatics::PlaySoundAtLocation(MyTarget, FallSound, Parameters.Location);
	return true;
}

UGCN_Weapon_Reload::UGCN_Weapon_Reload()
{
	GameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Weapon.Reload"));
}

bool UGCN_Weapon_Reload::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	USoundBase* Sound = const_cast<USoundBase*>(Cast<USoundBase>(Parameters.SourceObject.Get()));
	if (!Sound)
	{
		Sound = ReloadSound.Get();
	}
	if (!Sound) return false;

	USceneComponent* AttachComponent = Parameters.TargetAttachComponent.Get();
	if (!AttachComponent)
	{
		if (APDWeaponBase* Weapon = GetWeapon(MyTarget, Parameters))
		{
			AttachComponent = Weapon->GetWeaponMesh();
		}
	}

	if (AttachComponent)
	{
		UGameplayStatics::SpawnSoundAttached(Sound, AttachComponent);
	}
	else
	{
		UGameplayStatics::PlaySoundAtLocation(MyTarget, Sound, Parameters.Location);
	}
	return true;
}

UGCN_Weapon_ReloadEmpty::UGCN_Weapon_ReloadEmpty()
{
	GameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Weapon.ReloadEmpty"));
}

bool UGCN_Weapon_ReloadEmpty::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	APDRangedWeaponBase* Weapon = GetRangedWeapon(MyTarget, Parameters);
	USoundBase* Sound = Weapon && Weapon->ReloadEmptySound ? Weapon->ReloadEmptySound.Get() : ReloadEmptySound.Get();
	if (!Sound) return false;

	USceneComponent* AttachComponent = Parameters.TargetAttachComponent.Get();
	if (!AttachComponent && Weapon)
	{
		AttachComponent = Weapon->GetWeaponMesh();
	}

	if (AttachComponent)
	{
		UGameplayStatics::SpawnSoundAttached(Sound, AttachComponent);
	}
	else
	{
		UGameplayStatics::PlaySoundAtLocation(MyTarget, Sound, Parameters.Location);
	}

	return true;
}

UGCN_Item_Pickup::UGCN_Item_Pickup()
{
	GameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Item.Pickup"));
}

bool UGCN_Item_Pickup::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	USoundBase* Sound = PickupSound;
	if (!Sound)
	{
		Sound = const_cast<USoundBase*>(Cast<USoundBase>(Parameters.SourceObject.Get()));
	}

	if (!Sound)
	{
		return false;
	}

	UGameplayStatics::PlaySoundAtLocation(MyTarget, Sound, Parameters.Location);
	return true;
}

UGCN_Ability_Bombing::UGCN_Ability_Bombing()
{
	GameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Ability.Bombing"));
}

bool UGCN_Ability_Bombing::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	const FVector Location = Parameters.Location;
	const FRotator Rotation = Parameters.Normal.IsNearlyZero()
		? FRotator::ZeroRotator
		: Parameters.Normal.Rotation();
	const float RadiusScale = Parameters.RawMagnitude > KINDA_SMALL_NUMBER
		? Parameters.RawMagnitude / 100.f
		: 1.f;

	if (BombingNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			MyTarget,
			BombingNiagara,
			Location,
			Rotation,
			FVector(RadiusScale),
			true,
			true,
			ENCPoolMethod::AutoRelease);
	}

	if (BombingParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			MyTarget,
			BombingParticle,
			Location,
			Rotation,
			FVector(RadiusScale));
	}

	if (BombingSound)
	{
		UGameplayStatics::PlaySoundAtLocation(MyTarget, BombingSound, Location);
	}

	return true;
}



UGCN_Weapon_Impact::UGCN_Weapon_Impact()
{
	GameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Weapon.Impact"));
}

bool UGCN_Weapon_Impact::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	APDRangedWeaponBase* Weapon = GetRangedWeapon(MyTarget, Parameters);
	if (!Weapon) return false;
	
	if (Weapon->HitImpactEffect)
	if (Weapon->HitImpactEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			MyTarget,
			Weapon->HitImpactEffect,
			Parameters.Location,
			Parameters.Normal.Rotation(),
			true);
	}



	const bool bIsPawn = Parameters.RawMagnitude >= 1.f;
	USoundBase* Sound  = bIsPawn ? Weapon->HitBodySound : Weapon->HitSurfaceSound;
	if (Sound)
		UGameplayStatics::PlaySoundAtLocation(MyTarget, Sound, Parameters.Location);

	return true;
}



UGCN_Weapon_Equip::UGCN_Weapon_Equip()
{
	GameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Weapon.Equip"));
}

bool UGCN_Weapon_Equip::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	APDWeaponBase* Weapon = GetWeapon(MyTarget, Parameters);
	if (!Weapon || !Weapon->EquipSound)
	{
		return false;
	}

	USceneComponent* AttachComponent = Parameters.TargetAttachComponent.Get();
	if (!AttachComponent && Weapon->GetWeaponMesh())
	{
		AttachComponent = Weapon->GetWeaponMesh();
	}

	if (AttachComponent)
	{
		UGameplayStatics::SpawnSoundAttached(Weapon->EquipSound, AttachComponent);
	}
	else
	{
		UGameplayStatics::PlaySoundAtLocation(MyTarget, Weapon->EquipSound, Parameters.Location);
	}
	return true;
}



UGCN_Weapon_Swing::UGCN_Weapon_Swing()
{
	GameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Weapon.Swing"));
}

bool UGCN_Weapon_Swing::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	APDMeleeWeaponBase* Weapon = GetMeleeWeapon(MyTarget, Parameters);
	if (!Weapon || !Weapon->GetSwingSound()) return false;

	APDPlayerCharacter* Char = GetPDChar(MyTarget);
	UGameplayStatics::SpawnSoundAttached(
		Weapon->GetSwingSound(),
		Char ? static_cast<USceneComponent*>(Char->GetMesh()) : MyTarget->GetRootComponent());

	return true;
}



UGCN_Weapon_MeleeHit::UGCN_Weapon_MeleeHit()
{
	GameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Weapon.MeleeHit"));
}

bool UGCN_Weapon_MeleeHit::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	APDMeleeWeaponBase* Weapon = GetMeleeWeapon(MyTarget, Parameters);
	if (!Weapon || !Weapon->GetHitSound()) return false;

	UGameplayStatics::PlaySoundAtLocation(MyTarget, Weapon->GetHitSound(), Parameters.Location);
	return true;
}



UGCN_Character_HitReact::UGCN_Character_HitReact()
{
	GameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Character.HitReact"));
}

bool UGCN_Character_HitReact::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	ACharacter* Char = Cast<ACharacter>(MyTarget);
	if (!Char) return false;

	UPDAnimInstance* AnimInst = Cast<UPDAnimInstance>(Char->GetMesh()->GetAnimInstance());
	if (AnimInst)
	{
		AnimInst->PlayHitReaction();
	}

	if (APDCharacterBase* DamageTarget = Cast<APDCharacterBase>(MyTarget))
	{
		DamageTarget->OnDamageFeedback(
			Parameters.RawMagnitude,
			Parameters.Location,
			Parameters.Instigator.Get());
	}

	return true;
}



UGCN_Character_Bleeding::UGCN_Character_Bleeding()
{
	GameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Character.Bleeding"));
}

bool UGCN_Character_Bleeding::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	// TODO: 출혈 파티클/사운드. 지속형 필요 시 GCN_Actor 서브클래스로 전환.
	return true;
}
