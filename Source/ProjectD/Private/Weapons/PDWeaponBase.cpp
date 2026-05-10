#include "Weapons/PDWeaponBase.h"
#include "Weapons/PDShellActor.h"
#include "Weapons/PDMagazineActor.h"
#include "Characters/PDPlayerCharacter.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

#include "Animation/AnimInstance.h"
#include "Interfaces/PDDamageable.h"

APDWeaponBase::APDWeaponBase()
{
    PrimaryActorTick.bCanEverTick = false;

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    SetRootComponent(WeaponMesh);

    PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
    PickupCollision->SetupAttachment(RootComponent);
    PickupCollision->SetSphereRadius(80.f);
    PickupCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    MagazineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagazineMesh"));
    MagazineMesh->SetupAttachment(WeaponMesh, MagazineSocketName);
    MagazineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APDWeaponBase::BeginPlay()
{
    Super::BeginPlay();
    if (LevelStats.IsValidIndex(0))
        CurrentAmmo = LevelStats[0].MaxAmmo;

    if (bIsDropped)
    {
        PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }
}

void APDWeaponBase::Interact_Implementation(AActor* Interactor)
{
    if (WeaponOwner.IsValid()) return;

    APDPlayerCharacter* Player = Cast<APDPlayerCharacter>(Interactor);
    if (!Player) return;

    Player->PickupWeapon(this);
    SetDropped(false);
}

void APDWeaponBase::Fire_Implementation() {}

void APDWeaponBase::Reload_Implementation()
{
    if (bIsReloading) return;
    if (CurrentAmmo >= GetCurrentStats().MaxAmmo) return;

    bIsReloading = true;

    if (ReloadMontage)
    {
        PlayWeaponMontage(ReloadMontage);
        BindMontageEndedForReload(ReloadMontage);
    }
    else
    {
        GetWorldTimerManager().SetTimer(
            ReloadHandle, this,
            &APDWeaponBase::FinishReload,
            GetCurrentStats().ReloadTime, false);
    }
}

void APDWeaponBase::OnEquip_Implementation(AActor* NewOwner)
{
    WeaponOwner = NewOwner;
    SetOwner(NewOwner);
    WeaponMesh->SetSimulatePhysics(false);
    PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (CurrentAmmo == 0 && !LevelStats.IsEmpty())
        CurrentAmmo = LevelStats[0].MaxAmmo;
}

void APDWeaponBase::OnUnequip_Implementation()
{
    WeaponOwner = nullptr;
    GetWorldTimerManager().ClearTimer(FireCooldownHandle);
    GetWorldTimerManager().ClearTimer(ReloadHandle);
    bIsReloading = false;
    bCanFire = true;
}

void APDWeaponBase::EjectShell_Implementation()
{
    if (!ShellActorClass || !GetWorld()) return;

    FName Socket = WeaponMesh->DoesSocketExist(EjectionPortSocket)
        ? EjectionPortSocket : MuzzleSocketName;

    FTransform TM = WeaponMesh->GetSocketTransform(Socket);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (APDShellActor* Shell = GetWorld()->SpawnActor<APDShellActor>(
        ShellActorClass, TM.GetLocation(), TM.Rotator(), Params))
    {
        FVector Dir = TM.TransformVectorNoScale(ShellEjectLocalDir.GetSafeNormal());
        Shell->Launch(Dir * ShellEjectSpeed * FMath::RandRange(0.9f, 1.1f));
    }
}

void APDWeaponBase::DropMagazine_Implementation()
{
    if (!MagazineActorClass || !GetWorld()) return;

    if (MagazineMesh) MagazineMesh->SetVisibility(false);

    FTransform MagTM = MagazineMesh
        ? MagazineMesh->GetComponentTransform()
        : WeaponMesh->GetSocketTransform(MagazineSocketName);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (APDMagazineActor* Mag = GetWorld()->SpawnActor<APDMagazineActor>(
        MagazineActorClass, MagTM.GetLocation(), MagTM.Rotator(), Params))
    {
        if (MagazineMesh && MagazineMesh->GetStaticMesh())
            Mag->InitFromWeapon(MagazineMesh->GetStaticMesh(), MagazineMesh->GetMaterial(0));
        Mag->Drop();
    }
}

void APDWeaponBase::AttachNewMagazine_Implementation()
{
    if (MagazineMesh) MagazineMesh->SetVisibility(true);
}

// 볼트/슬러그 — BP 오버라이드용 빈 구현
void APDWeaponBase::OnBoltPulled_Implementation() {}
void APDWeaponBase::OnBoltReleased_Implementation() {}
void APDWeaponBase::OnShellInserted_Implementation() {}

void APDWeaponBase::UpgradeLevel()
{
    SetLevel(CurrentLevel + 1);
}

void APDWeaponBase::SetLevel(int32 NewLevel)
{
    CurrentLevel = FMath::Clamp(NewLevel, 1, LevelStats.Num());
    CurrentAmmo = GetCurrentStats().MaxAmmo;
    OnWeaponLevelChanged.Broadcast(this, CurrentLevel);
}

void APDWeaponBase::SetDropped(bool bDropped)
{
    bIsDropped = bDropped;

    if (bDropped)
    {
        PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        WeaponMesh->SetSimulatePhysics(true);
    }
    else
    {
        PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        WeaponMesh->SetSimulatePhysics(false);
    }
}

const FWeaponLevelStats& APDWeaponBase::GetCurrentStats() const
{
    static FWeaponLevelStats DefaultStats;
    if (LevelStats.IsEmpty()) return DefaultStats;
    return LevelStats[FMath::Clamp(CurrentLevel - 1, 0, LevelStats.Num() - 1)];
}

bool APDWeaponBase::CanFire() const
{
    return bCanFire && !bIsReloading && CurrentAmmo > 0;
}

void APDWeaponBase::ApplyDamage(AActor* HitActor, float DamageAmount)
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

void APDWeaponBase::PostFire()
{
    --CurrentAmmo;
    bCanFire = false;
    OnWeaponFired.Broadcast(this);

    ApplyRecoil();

    GetWorldTimerManager().SetTimer(
        FireCooldownHandle, this,
        &APDWeaponBase::ResetFireCooldown,
        GetCurrentStats().FireRate, false);
}

void APDWeaponBase::ResetFireCooldown()
{
    bCanFire = true;
}

void APDWeaponBase::FinishReload()
{
    CurrentAmmo = GetCurrentStats().MaxAmmo;
    bIsReloading = false;
    OnWeaponReloaded.Broadcast(this);
}

void APDWeaponBase::PlayWeaponMontage(UAnimMontage* Montage, FName StartSection)
{
    if (!Montage) return;
    UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
    if (!AnimInst) return;

    AnimInst->Montage_Play(Montage);
    if (StartSection != NAME_None)
        AnimInst->Montage_JumpToSection(StartSection, Montage);
}

bool APDWeaponBase::IsPlayingMontage(UAnimMontage* Montage) const
{
    UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
    return AnimInst ? AnimInst->Montage_IsPlaying(Montage) : false;
}

void APDWeaponBase::StopWeaponMontage(UAnimMontage* Montage)
{
    UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
    if (AnimInst && Montage)
        AnimInst->Montage_Stop(0.15f, Montage);
}

void APDWeaponBase::BindMontageEndedForReload(UAnimMontage* Montage)
{
    UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
    if (!AnimInst) return;

    FOnMontageEnded EndedDelegate;
    EndedDelegate.BindUObject(this, &APDWeaponBase::OnReloadMontageEnded);
    AnimInst->Montage_SetEndDelegate(EndedDelegate, Montage);
}

void APDWeaponBase::OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (!bInterrupted)
        FinishReload();
    else
        bIsReloading = false;
}

// 반동로직
void APDWeaponBase::ApplyRecoil()
{
    // 1. 카메라 쉐이크
    if (FireCameraShakeClass)
    {
        if (APlayerController* PC = GetOwnerPlayerController())
            PC->PlayerCameraManager->StartCameraShake(FireCameraShakeClass);
    }

    // 2. 스프레드 누적
    CurrentRecoilSpread = FMath::Min(
        CurrentRecoilSpread + RecoilSpreadPerShot,
        MaxRecoilSpread);

    // 스프레드 회복 타이머 (재시작)
    GetWorldTimerManager().SetTimer(
        SpreadRecoveryHandle, this,
        &APDWeaponBase::TickSpreadRecovery,
        0.016f, true);  // ~60fps

   
    if (WeaponMesh)
    {
        // 원본 회전 저장 (처음 발사 시)
        if (!GetWorldTimerManager().IsTimerActive(MeshRecoilRecoveryHandle))
            OriginalMeshRelRotation = WeaponMesh->GetRelativeRotation();

        
        FRotator KickedRot = WeaponMesh->GetRelativeRotation() + MeshRecoilKick;
        WeaponMesh->SetRelativeRotation(KickedRot);

        // 메시 복구 타이머
        GetWorldTimerManager().SetTimer(
            MeshRecoilRecoveryHandle, this,
            &APDWeaponBase::TickMeshRecoilRecovery,
            0.016f, true);
    }
}

void APDWeaponBase::TickSpreadRecovery()
{
    CurrentRecoilSpread -= RecoilRecoveryRate * 0.016f;

    if (CurrentRecoilSpread <= 0.f)
    {
        CurrentRecoilSpread = 0.f;
        GetWorldTimerManager().ClearTimer(SpreadRecoveryHandle);
    }
}
// 메시 반동 복구
void APDWeaponBase::TickMeshRecoilRecovery()
{
    if (!WeaponMesh) return;

    FRotator Current = WeaponMesh->GetRelativeRotation();
    FRotator Target = OriginalMeshRelRotation;

    FRotator NewRot = FMath::RInterpTo(Current, Target,
        0.016f, MeshRecoilRecoverySpeed);

    WeaponMesh->SetRelativeRotation(NewRot);

    // 거의 다 돌아왔으면 타이머 종료
    if (Current.Equals(Target, 0.1f))
    {
        WeaponMesh->SetRelativeRotation(Target);
        GetWorldTimerManager().ClearTimer(MeshRecoilRecoveryHandle);
    }
}

APlayerController* APDWeaponBase::GetOwnerPlayerController() const
{
    if (!WeaponOwner.IsValid()) return nullptr;
    return Cast<APlayerController>(WeaponOwner->GetInstigatorController());
}