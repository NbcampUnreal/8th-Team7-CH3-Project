#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Weapons/PDCartridge.h"

#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"

#include "Animation/AnimInstance.h"
#include "Interfaces/PDDamageable.h"
#include "Items/PDInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Core/PDPlayerController.h"
#include "Perception/AISense_Hearing.h"

APDRangedWeaponBase::APDRangedWeaponBase()
{
}

void APDRangedWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	// CurrentAmmo가 BP에서 직접 설정된 경우 그대로 사용, 0이면 MaxAmmo로 채움
	if (CurrentAmmo == 0 && LevelStats.IsValidIndex(0))
		CurrentAmmo = LevelStats[0].MaxAmmo;
}

void APDRangedWeaponBase::SetCurrentAmmo(int32 NewAmmo)
{
	const int32 Max = LevelStats.IsValidIndex(0) ? GetCurrentStats().MaxAmmo : 0;
	CurrentAmmo = Max > 0 ? FMath::Clamp(NewAmmo, 0, Max) : FMath::Max(0, NewAmmo);
}

void APDRangedWeaponBase::Fire_Implementation() {}

void APDRangedWeaponBase::Reload_Implementation()
{
	if (bIsReloading) return;
	if (CurrentAmmo >= GetCurrentStats().MaxAmmo) return;
	if (!HasAmmoToReload()) return;

	bIsReloading = true;

	// 캐릭터 애니메이션은 AnimInstance가 이 델리게이트를 구독해서 처리
	OnWeaponReloadStarted.Broadcast(this);

	if (ReloadMontage && WeaponMesh && WeaponMesh->GetAnimInstance())
	{
		PlayWeaponMontage(ReloadMontage);
		BindMontageEndedForReload(ReloadMontage);
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			ReloadHandle, this,
			&APDRangedWeaponBase::FinishReload,
			GetCurrentStats().ReloadTime, false);
	}
}

void APDRangedWeaponBase::OnEquip_Implementation(AActor* NewOwner)
{
	Super::OnEquip_Implementation(NewOwner);

	// 풀충탄 fallback 제거: 호출자(TryAutoEquipWeaponSlot 등)가 spawn 직후 명시적으로
	// CurrentAmmo를 설정한다. BeginPlay가 fresh actor의 0→Max를 처리하므로 여기선 불필요.
	// 빈 매그(CurrentAmmo=0) 영속화 보존을 위해 이 경로에서 덮어쓰지 않음.

	// 캐릭터 Equip 몽타주는 PDAnimInstance가 OnWeaponEquipped에서 담당.
}

void APDRangedWeaponBase::OnUnequip_Implementation()
{
	Super::OnUnequip_Implementation();

	GetWorldTimerManager().ClearTimer(FireCooldownHandle);
	GetWorldTimerManager().ClearTimer(ReloadHandle);
	bIsReloading = false;
	bCanFire = true;
}

bool APDRangedWeaponBase::CanFire() const
{
	return bCanFire && !bIsReloading && CurrentAmmo > 0;
}

void APDRangedWeaponBase::ApplyDamage(AActor* HitActor, float DamageAmount)
{
	if (!HitActor) return;
	if (!HitActor->Implements<UPDDamageable>()) return;
	if (IPDDamageable::Execute_GetCurrentHealth(HitActor) <= 0.f) return;

	FPDDamageInfo DamageInfo;
	DamageInfo.BaseDamage = DamageAmount;
	DamageInfo.Instigator = GetWeaponOwner();
	DamageInfo.DamageTypeClass = nullptr;

	IPDDamageable::Execute_ApplyDamage(HitActor, DamageInfo);
}

void APDRangedWeaponBase::PostFire()
{
	--CurrentAmmo;
	bCanFire = false;
	OnWeaponFired.Broadcast(this);

	ApplyRecoil();

	// 총성 청각 자극 — 주변 AI 가 발사 위치/사수를 인지하도록.
	// Instigator 는 WeaponOwner(사수 Pawn) — Perception 측 affiliation 평가에 사용.
	if (GunshotNoiseRange > 0.f)
	{
		AActor* NoiseInstigator = WeaponOwner.IsValid() ? WeaponOwner.Get() : this;
		UAISense_Hearing::ReportNoiseEvent(
			GetWorld(),
			GetActorLocation(),
			/*Loudness=*/1.f,
			NoiseInstigator,
			GunshotNoiseRange,
			GunshotNoiseTag);
	}

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
	GetWorldTimerManager().ClearTimer(ReloadHandle);
	StopWeaponMontage(ReloadMontage);
	bIsReloading = false;
}

void APDRangedWeaponBase::FinishReload()
{
	const int32 MaxAmmo = GetCurrentStats().MaxAmmo;
	const int32 Needed = MaxAmmo - CurrentAmmo;

	// 무한 탄약: 인벤토리 무시하고 무조건 풀충.
	if (bInfiniteAmmo)
	{
		CurrentAmmo = MaxAmmo;
	}
	else if (!AmmoItemID.IsNone())
	{
		UPDInventoryComponent* Inv = GetOwnerInventory();
		if (Inv)
		{
			const int32 ToAdd = FMath::Min(Needed, GetAvailableAmmoCount());
			if (ToAdd > 0)
			{
				Inv->RemoveItem(AmmoItemID, ToAdd);
				CurrentAmmo += ToAdd;
			}
		}
		else { CurrentAmmo = MaxAmmo; }
	}
	else { CurrentAmmo = MaxAmmo; }

	bIsReloading = false;
	OnWeaponReloaded.Broadcast(this);
}

void APDRangedWeaponBase::PlayWeaponMontage(UAnimMontage* Montage, FName StartSection)
{
	if (!Montage) return;
	UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
	if (!AnimInst) return;

	AnimInst->Montage_Play(Montage);
	if (StartSection != NAME_None)
		AnimInst->Montage_JumpToSection(StartSection, Montage);
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
		bIsReloading = false;
}

void APDRangedWeaponBase::ApplyRecoil()
{
	// 에임 Yaw 오프셋 — PlayerController에 누적
	if (APDPlayerController* PDPC = Cast<APDPlayerController>(GetOwnerPlayerController()))
	{
		// 한 방향으로 누적(+방향). 회복은 TickRecoilRecovery에서 0으로 수렴.
		// 랜덤 좌우는 서로 상쇄되어 체감이 없으므로 단방향 누적 사용.
		const float CurrentOffset = PDPC->GetRecoilYawOffset();
		const float Remaining = MaxRecoilYaw - CurrentOffset;
		const float Delta = FMath::Min(RecoilYawPerShot, FMath::Max(0.f, Remaining));

		if (Delta > 0.f)
			PDPC->AddRecoilOffset(Delta);
	}
}

void APDRangedWeaponBase::PlayFireEffects()
{
	if (MuzzleFlashEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(
			MuzzleFlashEffect,
			WeaponMesh,
			MuzzleSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true);
	}

	if (FireSound)
	{
		UGameplayStatics::SpawnSoundAttached(
			FireSound,
			WeaponMesh,
			MuzzleSocketName);
	}
}

void APDRangedWeaponBase::SpawnBeamEffect(const FVector& Start, const FVector& End)
{
	if (!BeamParticle || !GetWorld()) return;

	// Beam emitter: 시작점에 스폰 후 Target 파라미터로 끝점 지정
	UParticleSystemComponent* BeamComp = UGameplayStatics::SpawnEmitterAtLocation(
		GetWorld(), BeamParticle, Start, FRotator::ZeroRotator, true);
	if (BeamComp)
		BeamComp->SetVectorParameter(FName("Target"), End);
}

void APDRangedWeaponBase::SpawnTracerEffect(const FVector& Start, const FVector& End)
{
	if (!TracerEffect || !GetWorld()) return;

	FVector Dir = (End - Start).GetSafeNormal();
	UGameplayStatics::SpawnEmitterAtLocation(
		GetWorld(), TracerEffect, Start, Dir.Rotation());
}

void APDRangedWeaponBase::SpawnImpactEffect(const FHitResult& Hit)
{
	if (!HitImpactEffect || !GetWorld()) return;

	UGameplayStatics::SpawnEmitterAtLocation(
		GetWorld(),
		HitImpactEffect,
		Hit.ImpactPoint,
		Hit.ImpactNormal.Rotation());
}

void APDRangedWeaponBase::PlayHitSound(const FHitResult& Hit)
{
	// 피격 대상이 Pawn이면 HitBodySound, 아니면 HitSurfaceSound
	USoundBase* Sound = Cast<APawn>(Hit.GetActor()) ? HitBodySound : HitSurfaceSound;
	if (Sound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, Hit.ImpactPoint);
}

void APDRangedWeaponBase::SpawnCartridge()
{
	if (!CartridgeClass || !WeaponMesh) return;
	if (!WeaponMesh->DoesSocketExist(CartridgeEjectSocketName)) return;

	const FTransform EjectTransform = WeaponMesh->GetSocketTransform(CartridgeEjectSocketName);
	GetWorld()->SpawnActor<APDCartridge>(
		CartridgeClass,
		EjectTransform.GetLocation(),
		EjectTransform.GetRotation().Rotator());
}

APlayerController* APDRangedWeaponBase::GetOwnerPlayerController() const
{
	if (!WeaponOwner.IsValid()) return nullptr;
	// WeaponOwner는 AActor*로 저장되어 있어 AActor::GetInstigatorController()가 호출됨.
	// 이 함수는 Actor의 Instigator 필드(미설정)를 통해 반환하므로 nullptr이 됨.
	// APawn::GetController()를 직접 사용해야 실제 PlayerController를 얻을 수 있다.
	if (APawn* OwnerPawn = Cast<APawn>(WeaponOwner.Get()))
		return Cast<APlayerController>(OwnerPawn->GetController());
	return nullptr;
}

UPDInventoryComponent* APDRangedWeaponBase::GetOwnerInventory() const
{
	if (!WeaponOwner.IsValid()) return nullptr;
	return WeaponOwner->FindComponentByClass<UPDInventoryComponent>();
}

bool APDRangedWeaponBase::HasAmmoToReload() const
{
	if (bInfiniteAmmo) return true;
	if (AmmoItemID.IsNone()) return true;
	UPDInventoryComponent* Inv = GetOwnerInventory();
	if (!Inv) return true;
	return Inv->HasItem(AmmoItemID, 1);
}

int32 APDRangedWeaponBase::GetAvailableAmmoCount() const
{
	if (AmmoItemID.IsNone()) return INT32_MAX;
	UPDInventoryComponent* Inv = GetOwnerInventory();
	if (!Inv) return INT32_MAX;
	int32 Total = 0;
	for (const FPDInventorySlot& Slot : Inv->Items)
		if (!Slot.IsEmpty() && Slot.ItemData.ItemID == AmmoItemID)
			Total += Slot.Quantity;
	return Total;
}
