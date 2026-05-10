// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/PDSniper.h"
#include "GameFramework/PlayerController.h"

APDSniper::APDSniper()
{
    WeaponType = EWeaponType::Sniper;

    // 저격총 줌 FOV 설정
    DefaultFOV = 90.f;
    ZoomedFOV = 40.f;

    LevelStats.Add({ 80.f, 1.5f,  8000.f, 5, 3.0f, 1.0f }); // Lv1
    LevelStats.Add({ 120.f, 1.3f, 10000.f, 5, 2.7f, 1.0f }); // Lv2
    LevelStats.Add({ 180.f, 1.2f, 15000.f, 5, 2.5f, 1.0f }); // Lv3
}

void APDSniper::Fire_Implementation()
{
    if (!CanFire()) return;
    if (!ProjectileClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("PDSniper: ProjectileClass 미설정"));
        return;
    }

    SpawnProjectile(CanPenetrate());
    PostFire();

    FTimerHandle T_Shell;
    GetWorldTimerManager().SetTimer(T_Shell, FTimerDelegate::CreateLambda([this]()
        {
            EjectShell();
        }), 0.3f, false);

    if (BoltActionMontage)
    {
        PlayWeaponMontage(BoltActionMontage);
        UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
        if (AnimInst)
        {
            FOnMontageEnded EndDelegate;
            EndDelegate.BindUObject(this, &APDSniper::OnBoltActionMontageEnded);
            AnimInst->Montage_SetEndDelegate(EndDelegate, BoltActionMontage);
        }
    }
}
void APDSniper::Reload_Implementation()
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

void APDSniper::OnBoltActionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 필요 시 BP에서 추가 로직 (사운드, 이펙트 등)
}

void APDSniper::ToggleZoom()
{
    bIsZoomed = !bIsZoomed;

    AActor* WeaponOwnerActor = GetWeaponOwner();
    if (!WeaponOwnerActor) return;

    APlayerController* PC = Cast<APlayerController>(WeaponOwnerActor->GetInstigatorController());
    if (!PC || !PC->PlayerCameraManager) return;

    PC->PlayerCameraManager->SetFOV(bIsZoomed ? ZoomedFOV : DefaultFOV);
    OnScopeToggled.Broadcast(bIsZoomed);
}

void APDSniper::SpawnProjectile(bool bPenetrate)
{
    AActor* WeaponOwnerActor = GetWeaponOwner();
    if (!WeaponOwnerActor) return;

    FVector Start = WeaponMesh->DoesSocketExist(MuzzleSocketName)
        ? WeaponMesh->GetSocketLocation(MuzzleSocketName)
        : WeaponOwnerActor->GetActorLocation();

    FVector Forward = GetAimDirection();
    FRotator SpawnRot = Forward.Rotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = GetInstigator();
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    APDProjectile* Projectile = GetWorld()->SpawnActor<APDProjectile>(
        ProjectileClass, Start, SpawnRot, SpawnParams);

    if (Projectile)
        Projectile->InitProjectile(GetCurrentStats().Damage, WeaponOwnerActor, bPenetrate);
}

FVector APDSniper::GetAimDirection() const
{
    AActor* WeaponOwnerActor = GetWeaponOwner();
    if (!WeaponOwnerActor) return FVector::ForwardVector;

    APlayerController* PC = Cast<APlayerController>(WeaponOwnerActor->GetInstigatorController());

    FVector Start = WeaponMesh->DoesSocketExist(MuzzleSocketName)
        ? WeaponMesh->GetSocketLocation(MuzzleSocketName)
        : WeaponOwnerActor->GetActorLocation();

    if (PC)
    {
        // 1순위: 커서가 Pawn 위 → 부위 직접 조준
        FHitResult PawnHit;
        if (PC->GetHitResultUnderCursorForObjects(
            { UEngineTypes::ConvertToObjectType(ECC_Pawn) }, true, PawnHit)
            && PawnHit.GetActor() && PawnHit.GetActor() != WeaponOwnerActor)
        {
            FVector Dir = PawnHit.Location - Start;
            if (!Dir.IsNearlyZero()) return Dir.GetSafeNormal();
        }

        // 2순위: 지면 커서 → Z 유지
        FHitResult CursorHit;
        if (PC->GetHitResultUnderCursor(ECC_Visibility, true, CursorHit))
        {
            FVector Dir = CursorHit.Location - Start;
            if (!Dir.IsNearlyZero()) return Dir.GetSafeNormal();
        }
    }

    return WeaponOwnerActor->GetActorForwardVector();
}

bool APDSniper::CanPenetrate() const
{
    return PenetrationPerLevel[FMath::Clamp(CurrentLevel - 1, 0, PenetrationPerLevel.Num() - 1)];
}
