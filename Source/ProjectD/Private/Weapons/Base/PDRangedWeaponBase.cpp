#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Characters/PDPlayerCharacter.h"

#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/PDAnimInstance.h"
#include "Interfaces/PDDamageable.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Core/PDPlayerComponentResolver.h"
#include "Core/PDPlayerController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTag/PDGameplayTags.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

APDRangedWeaponBase::APDRangedWeaponBase()
{
}

void APDRangedWeaponBase::BeginPlay()
{
	Super::BeginPlay();


	if (CurrentAmmo == 0 && LevelStats.IsValidIndex(0))
		CurrentAmmo = LevelStats[0].MaxAmmo;
}

void APDRangedWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APDRangedWeaponBase, CurrentAmmo);
	DOREPLIFETIME(APDRangedWeaponBase, ReserveAmmo);
	DOREPLIFETIME(APDRangedWeaponBase, bIsReloading);
}

void APDRangedWeaponBase::SetCurrentAmmo(int32 NewAmmo)
{
	const int32 Max = LevelStats.IsValidIndex(0) ? GetCurrentStats().MaxAmmo : 0;
	CurrentAmmo = Max > 0 ? FMath::Clamp(NewAmmo, 0, Max) : FMath::Max(0, NewAmmo);
}

void APDRangedWeaponBase::Fire_Implementation() {}

void APDRangedWeaponBase::Reload_Implementation()
{

	if (!HasAuthority())
	{
		return;
	}
	if (!CanReload())
	{
		if (!bIsReloading && CurrentAmmo < GetCurrentStats().MaxAmmo && !HasAmmoToReload())
		{
			ExecuteReloadEmptyCue();
		}
		return;
	}

	bIsReloading = true;
	BroadcastAmmoChanged();


	MulticastOnWeaponReloadStarted();

	const float ReloadDuration = GetReloadDuration();
	ExecuteReloadCue(ReloadDuration);

	if (ReloadDuration <= KINDA_SMALL_NUMBER)
	{
		FinishReload();
		return;
	}

	GetWorldTimerManager().SetTimer(
		ReloadHandle,
		this,
		&APDRangedWeaponBase::FinishReload,
		ReloadDuration,
		false);
}

void APDRangedWeaponBase::OnEquip_Implementation(AActor* NewOwner)
{
	Super::OnEquip_Implementation(NewOwner);

	// ??충탄 fallback ??거: ??출??TryAutoEquipWeaponSlot ??가 spawn 직후 명시??으??	// CurrentAmmo????정??다. BeginPlay가 fresh actor??0??Max??처리????????기??불필??
	// ??매그(CurrentAmmo=0) ??속??보존????해 ??경로??서 ???????? ??음.

	BroadcastAmmoChanged();


}

void APDRangedWeaponBase::OnUnequip_Implementation()
{
	Super::OnUnequip_Implementation();

	const bool bWasReloading = bIsReloading;
	GetWorldTimerManager().ClearTimer(FireCooldownHandle);
	GetWorldTimerManager().ClearTimer(ReloadHandle);
	bIsReloading = false;
	bCanFire = true;
	BroadcastAmmoChanged();

	if (HasAuthority() && bWasReloading)
	{
		OnWeaponReloaded.Broadcast(this);
	}
}

bool APDRangedWeaponBase::CanFire() const
{
	return bCanFire && !bIsReloading && CurrentAmmo > 0;
}

bool APDRangedWeaponBase::CanReload() const
{
	return !bIsReloading
		&& CurrentAmmo < GetCurrentStats().MaxAmmo
		&& HasAmmoToReload();
}

void APDRangedWeaponBase::RefreshAmmoChanged()
{
	BroadcastAmmoChanged();
}

void APDRangedWeaponBase::ApplyDamage(AActor* HitActor, float DamageAmount, const FHitResult& HitResult)
{

	if (!HitActor)
	{
		return;
	}
	if (!HitActor->Implements<UPDDamageable>())
	{
		return;
	}

	FPDDamageInfo DamageInfo;
	DamageInfo.BaseDamage = DamageAmount;
	DamageInfo.Instigator = GetWeaponOwner();
	DamageInfo.DamageTypeClass = nullptr;
	DamageInfo.HitResult = HitResult;


	IPDDamageable::Execute_ApplyDamage(HitActor, DamageInfo);
}

void APDRangedWeaponBase::PostFire()
{
	if (!HasAuthority()) return;
	--CurrentAmmo;
	bCanFire = false;
	BroadcastAmmoChanged();
	MulticastOnWeaponFired();

	ApplyRecoil();

	GetWorldTimerManager().SetTimer(
		FireCooldownHandle, this,
		&APDRangedWeaponBase::ResetFireCooldown,
		GetCurrentStats().FireRate, false);
}

void APDRangedWeaponBase::ResetFireCooldown()
{
	bCanFire = true;
}

void APDRangedWeaponBase::CancelReload()
{
	if (!HasAuthority()) return;
	GetWorldTimerManager().ClearTimer(ReloadHandle);
	StopWeaponMontage(ReloadMontage);
	if (const APawn* OwnerPawn = Cast<APawn>(WeaponOwner.Get()))
	{
		if (const ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerPawn))
		{
			if (USkeletalMeshComponent* CharacterMesh = OwnerCharacter->GetMesh())
			{
				if (UPDAnimInstance* AnimInst = Cast<UPDAnimInstance>(CharacterMesh->GetAnimInstance()))
				{
					AnimInst->StopReloadMontageForWeapon(this);
				}
			}
		}
	}
	bIsReloading = false;
	BroadcastAmmoChanged();
}

void APDRangedWeaponBase::FinishReload()
{
	if (!HasAuthority()) return;
	const int32 MaxAmmo = GetCurrentStats().MaxAmmo;
	const int32 Needed = MaxAmmo - CurrentAmmo;
	if (Needed <= 0)
	{
		bIsReloading = false;
		BroadcastAmmoChanged();
		OnWeaponReloaded.Broadcast(this);
		return;
	}

	// 무한 ??약: ??벤??리 무시??고 무조??????
	if (bInfiniteAmmo)
	{
		CurrentAmmo = MaxAmmo;
	}
	else if (!AmmoItemID.IsNone())
	{
		UPDInventoryComponent* Inv = GetOwnerInventory();
		int32 InventoryAmmo = 0;
		if (Inv)
		{
			for (const FPDInventorySlot& Slot : Inv->Items)
			{
				if (!Slot.IsEmpty() && Slot.ItemData.ItemID == AmmoItemID)
				{
					InventoryAmmo += Slot.Quantity;
				}
			}
		}
		if (Inv && InventoryAmmo > 0)
		{
			const int32 ToAdd = FMath::Min(Needed, InventoryAmmo);
			if (ToAdd > 0)
			{
				Inv->RemoveItem(AmmoItemID, ToAdd);
				CurrentAmmo += ToAdd;
			}
		}
		else
		{
			const int32 ToAdd = FMath::Min(Needed, ReserveAmmo);
			ReserveAmmo -= ToAdd;
			CurrentAmmo += ToAdd;
		}
	}
	else
	{
		const int32 ToAdd = FMath::Min(Needed, ReserveAmmo);
		ReserveAmmo -= ToAdd;
		CurrentAmmo += ToAdd;
	}

	bIsReloading = false;
	BroadcastAmmoChanged();
	OnWeaponReloaded.Broadcast(this);
}

void APDRangedWeaponBase::PlayWeaponMontage(UAnimMontage* Montage, FName StartSection)
{
	if (!Montage) return;
	UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
	if (!AnimInst) return;

	AnimInst->Montage_Play(Montage);
	if (StartSection != NAME_None && Montage->GetSectionIndex(StartSection) != INDEX_NONE)
	{
		AnimInst->Montage_JumpToSection(StartSection, Montage);
	}
}

bool APDRangedWeaponBase::IsPlayingMontage(UAnimMontage* Montage) const
{
	UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
	return AnimInst ? AnimInst->Montage_IsPlaying(Montage) : false;
}

void APDRangedWeaponBase::StopWeaponMontage(UAnimMontage* Montage)
{
	UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
	if (AnimInst && Montage)
		AnimInst->Montage_Stop(0.15f, Montage);
}

void APDRangedWeaponBase::BindMontageEndedForReload(UAnimMontage* Montage)
{
	UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
	if (!AnimInst) return;

	FOnMontageEnded EndedDelegate;
	EndedDelegate.BindUObject(this, &APDRangedWeaponBase::OnReloadMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndedDelegate, Montage);
}

void APDRangedWeaponBase::OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bInterrupted)
		FinishReload();
	else
	{
		bIsReloading = false;
		BroadcastAmmoChanged();
	}
}

void APDRangedWeaponBase::MulticastOnWeaponFired_Implementation()
{
	OnWeaponFired.Broadcast(this);
}

void APDRangedWeaponBase::MulticastOnWeaponReloadStarted_Implementation()
{
	OnWeaponReloadStarted.Broadcast(this);
}

void APDRangedWeaponBase::ApplyRecoil()
{

	if (APDPlayerController* PDPC = Cast<APDPlayerController>(GetOwnerPlayerController()))
	{
		if (PDPC->IsLocalController())
		{
			PDPC->AddRecoilOffset(RecoilYawPerShot);
		}
		else
		{
			PDPC->ClientAddRecoilOffset(RecoilYawPerShot);
		}
	}
}

void APDRangedWeaponBase::ExecuteFireCue(const FVector& MuzzleLoc, const FVector& TraceEnd)
{
	UAbilitySystemComponent* ASCComp = GetOwnerASC();
	if (!ASCComp)
	{
		return;
	}



	FGameplayCueParameters Params;
	Params.Location              = MuzzleLoc;
	Params.TargetAttachComponent = WeaponMesh;
	Params.SourceObject          = this;
	Params.EffectCauser          = this;
	Params.Instigator            = GetWeaponOwner();
	const bool bHasTracerTarget = !TraceEnd.IsZero() && !TraceEnd.Equals(MuzzleLoc);
	Params.Normal                = bHasTracerTarget ? (TraceEnd - MuzzleLoc).GetSafeNormal() : FVector::ZeroVector;
	Params.RawMagnitude          = bHasTracerTarget ? FVector::Dist(MuzzleLoc, TraceEnd) : 0.f;
	ASCComp->ExecuteGameplayCue(PDGameplayTags::GameplayCue_Weapon_Fire, Params);
}

void APDRangedWeaponBase::ExecuteImpactCue(const FHitResult& Hit)
{
	UAbilitySystemComponent* ASCComp = GetOwnerASC();
	if (!ASCComp) return;




	FGameplayCueParameters Params;
	Params.Location          = Hit.ImpactPoint;
	Params.Normal            = Hit.ImpactNormal;
	Params.RawMagnitude      = Cast<APawn>(Hit.GetActor()) ? 1.f : 0.f;
	Params.PhysicalMaterial  = Hit.PhysMaterial;
	Params.SourceObject      = this;
	Params.EffectCauser      = this;
	Params.Instigator        = GetWeaponOwner();
	ASCComp->ExecuteGameplayCue(PDGameplayTags::GameplayCue_Weapon_Impact, Params);
}

void APDRangedWeaponBase::ExecuteReloadCue(float ReloadDuration)
{
	UAbilitySystemComponent* ASCComp = GetOwnerASC();
	if (!ASCComp) return;

	FGameplayCueParameters Params;
	Params.Location = WeaponMesh ? WeaponMesh->GetComponentLocation() : GetActorLocation();
	Params.TargetAttachComponent = WeaponMesh;
	Params.SourceObject = this;
	Params.EffectCauser = this;
	Params.Instigator = GetWeaponOwner();
	Params.RawMagnitude = ReloadDuration;
	ASCComp->ExecuteGameplayCue(PDGameplayTags::GameplayCue_Weapon_Reload, Params);
}

void APDRangedWeaponBase::ExecuteReloadEmptyCue()
{
	UAbilitySystemComponent* ASCComp = GetOwnerASC();
	if (!ASCComp) return;

	FGameplayCueParameters Params;
	Params.Location = WeaponMesh ? WeaponMesh->GetComponentLocation() : GetActorLocation();
	Params.TargetAttachComponent = WeaponMesh;
	Params.SourceObject = this;
	Params.EffectCauser = this;
	Params.Instigator = GetWeaponOwner();
	ASCComp->ExecuteGameplayCue(PDGameplayTags::GameplayCue_Weapon_ReloadEmpty, Params);
}

float APDRangedWeaponBase::GetReloadDuration() const
{
	if (const APawn* OwnerPawn = Cast<APawn>(WeaponOwner.Get()))
	{
		if (const ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerPawn))
		{
			if (const USkeletalMeshComponent* CharacterMesh = OwnerCharacter->GetMesh())
			{
				if (const UPDAnimInstance* AnimInst = Cast<UPDAnimInstance>(CharacterMesh->GetAnimInstance()))
				{
					const float AnimReloadDuration = AnimInst->GetReloadMontageDurationForWeapon(const_cast<APDRangedWeaponBase*>(this));
					if (AnimReloadDuration > 0.f)
					{
						return AnimReloadDuration;
					}
				}
			}
		}
	}

	if (ReloadMontage)
	{
		return ReloadMontage->GetPlayLength();
	}

	return GetCurrentStats().ReloadTime;
}



APlayerController* APDRangedWeaponBase::GetOwnerPlayerController() const
{
	if (!IsValid(WeaponOwner)) return nullptr;



	if (APawn* OwnerPawn = Cast<APawn>(WeaponOwner.Get()))
		return Cast<APlayerController>(OwnerPawn->GetController());
	return nullptr;
}

UPDInventoryComponent* APDRangedWeaponBase::GetOwnerInventory() const
{
	return IsValid(WeaponOwner) ? FPDPlayerComponentResolver::ResolveInventory(WeaponOwner.Get()) : nullptr;
}

UAbilitySystemComponent* APDRangedWeaponBase::GetOwnerASC() const
{
	if (!IsValid(WeaponOwner)) return nullptr;
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(WeaponOwner.Get());
}

bool APDRangedWeaponBase::HasAmmoToReload() const
{
	if (bInfiniteAmmo) return true;
	return GetAvailableAmmoCount() > 0;
}

int32 APDRangedWeaponBase::GetAvailableAmmoCount() const
{
	if (AmmoItemID.IsNone())
	{
		return ReserveAmmo;
	}

	const UPDInventoryComponent* Inv = GetOwnerInventory();
	if (!Inv) return ReserveAmmo;

	int32 Total = 0;
	for (const FPDInventorySlot& Slot : Inv->Items)
	{
		if (!Slot.IsEmpty() && Slot.ItemData.ItemID == AmmoItemID)
		{
			Total += Slot.Quantity;
		}
	}
	return Total > 0 ? Total : ReserveAmmo;
}

void APDRangedWeaponBase::BroadcastAmmoChanged()
{
	OnWeaponAmmoChanged.Broadcast(
		this,
		CurrentAmmo,
		GetCurrentStats().MaxAmmo,
		GetAvailableAmmoCount(),
		bIsReloading);
}

void APDRangedWeaponBase::OnRep_CurrentAmmo()
{
	BroadcastAmmoChanged();
}

void APDRangedWeaponBase::OnRep_ReserveAmmo()
{
	BroadcastAmmoChanged();
}

void APDRangedWeaponBase::OnRep_IsReloading()
{
	BroadcastAmmoChanged();
}
