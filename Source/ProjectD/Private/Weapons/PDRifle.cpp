#include "Weapons/PDRifle.h"
#include "Core/PDPlayerController.h"
#include "DrawDebugHelpers.h"

APDRifle::APDRifle()
{
    WeaponType = EWeaponType::Rifle;

    LevelStats.Add({ 20.f, 0.12f, 1500.f, 30, 2.0f, 0.90f }); // Lv1
    LevelStats.Add({ 28.f, 0.10f, 1700.f, 35, 1.8f, 0.93f }); // Lv2
    LevelStats.Add({ 38.f, 0.08f, 2000.f, 40, 1.5f, 0.96f }); // Lv3
}

void APDRifle::Fire_Implementation()
{
    UE_LOG(LogTemp, Warning, TEXT("Fire called! CanFire: %d, Ammo: %d"),
        CanFire(), CurrentAmmo);

    if (!CanFire()) return;

    FHitResult Hit;
    if (PerformLineTrace(Hit))
        ApplyDamage(Hit.GetActor(), GetCurrentStats().Damage);

    PostFire();

    FTimerHandle T_Shell;
    GetWorldTimerManager().SetTimer(T_Shell, FTimerDelegate::CreateLambda([this]()
        {
            EjectShell();
        }), 0.05f, false);

    if (CurrentAmmo <= 0) StopFire();
}

void APDRifle::Reload_Implementation()
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
        // 몽타주 미설정 시 폴백
        GetWorldTimerManager().SetTimer(
            ReloadHandle, this,
            &APDWeaponBase::FinishReload,
            GetCurrentStats().ReloadTime, false);
    }
}

void APDRifle::StartFire()
{
    if (bIsFiring) return;
    bIsFiring = true;

    Fire();

    if (FireMode == EFireMode::Auto)
    {
        GetWorldTimerManager().SetTimer(
            AutoFireHandle, this,
            &APDRifle::Fire_Implementation,
            GetCurrentStats().FireRate, true);
    }
}

void APDRifle::StopFire()
{
    bIsFiring = false;
    GetWorldTimerManager().ClearTimer(AutoFireHandle);
}

void APDRifle::ToggleFireMode()
{
    FireMode = (FireMode == EFireMode::Auto)
        ? EFireMode::Single : EFireMode::Auto;

    GetWorldTimerManager().ClearTimer(AutoFireHandle);
    OnFireModeChanged.Broadcast(FireMode);
}

void APDRifle::OnUnequip_Implementation()
{
    StopFire();
    Super::OnUnequip_Implementation();
}
bool APDRifle::PerformLineTrace(FHitResult& OutHit)
{
    const FWeaponLevelStats& Stats = GetCurrentStats();
    AActor* WeaponOwnerActor = GetWeaponOwner();
    if (!WeaponOwnerActor) return false;

    APlayerController* PC = Cast<APlayerController>(WeaponOwnerActor->GetInstigatorController());
    if (!PC) return false;

    FVector Start = WeaponMesh->DoesSocketExist(MuzzleSocketName)
        ? WeaponMesh->GetSocketLocation(MuzzleSocketName)
        : WeaponOwnerActor->GetActorLocation();

    FVector AimDir = WeaponOwnerActor->GetActorForwardVector();

    // 1순위: 커서가 Pawn 위 → 부위 직접 조준
    FHitResult PawnHit;
    if (PC->GetHitResultUnderCursorForObjects(
        { UEngineTypes::ConvertToObjectType(ECC_Pawn) }, true, PawnHit)
        && PawnHit.GetActor() && PawnHit.GetActor() != WeaponOwnerActor)
    {
        FVector Dir = PawnHit.Location - Start;
        if (!Dir.IsNearlyZero()) AimDir = Dir.GetSafeNormal();
    }
    // 2순위: 지면 커서 → Z 유지
    else
    {
        FHitResult CursorHit;
        if (PC->GetHitResultUnderCursor(ECC_Visibility, true, CursorHit))
        {
            FVector Dir = CursorHit.Location - Start;
            if (!Dir.IsNearlyZero()) AimDir = Dir.GetSafeNormal();
        }
    }
    // 연사시 탄퍼짐
    const float BaseSpread = (1.f - Stats.Accuracy) * 5.f;
    const float TotalSpread = FMath::DegreesToRadians(BaseSpread + CurrentRecoilSpread);
    const FVector ShootDir = FMath::VRandCone(AimDir, TotalSpread);
    const FVector End = Start + ShootDir * Stats.Range;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(WeaponOwnerActor);
    Params.bTraceComplex = true;

    const bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Pawn, Params);
    DrawDebugLine(GetWorld(), Start, bHit ? OutHit.Location : End,
        bHit ? FColor::Red : FColor::Green, false, 1.f, 0, 1.f);

    return bHit;
}