#include "Weapons/PDRifle.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Core/PDPlayerController.h"

APDRifle::APDRifle()
{
    WeaponType = EWeaponType::Rifle;
    bFullAuto  = true;

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
    {
        ApplyDamage(Hit.GetActor(), GetCurrentStats().Damage);
        SpawnImpactEffect(Hit);
        PlayHitSound(Hit);
    }

    PlayFireEffects();
    SpawnCartridge();
    PlayWeaponMontage(FireMontage);
    PostFire();
}

void APDRifle::ToggleFireMode()
{
    FireMode = (FireMode == EFireMode::Auto)
        ? EFireMode::Single : EFireMode::Auto;

    OnFireModeChanged.Broadcast(FireMode);
}

bool APDRifle::PerformLineTrace(FHitResult& OutHit)
{
    const FWeaponLevelStats& Stats = GetCurrentStats();
    AActor* WeaponOwnerActor = GetWeaponOwner();
    if (!WeaponOwnerActor) return false;

    APlayerController* PC = nullptr;
    if (APawn* OwnerPawn = Cast<APawn>(WeaponOwnerActor))
        PC = Cast<APlayerController>(OwnerPawn->GetController());

    FVector Start = WeaponMesh->DoesSocketExist(MuzzleSocketName)
        ? WeaponMesh->GetSocketLocation(MuzzleSocketName)
        : WeaponOwnerActor->GetActorLocation();

    FVector AimDir = GetAimDirectionFromOwner(Start);

    float TraceLength = Stats.Range;
    if (PC)
    {
        FHitResult CursorHit;
        if (PC->GetHitResultUnderCursor(ECC_Visibility, true, CursorHit))
            TraceLength = FVector::Dist(Start, CursorHit.Location);
    }
    // 반동은 캐릭터 회전(RecoilYawOffset)으로 처리됨.
    // 총알 스프레드는 무기 Accuracy 기반 랜덤만 남김.
    const float TotalSpread = FMath::DegreesToRadians((1.f - Stats.Accuracy) * 5.f);
    const FVector ShootDir = FMath::VRandCone(AimDir, TotalSpread);
    const FVector End = Start + ShootDir * TraceLength;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(WeaponOwnerActor);
    Params.bTraceComplex = true;

    const bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Pawn, Params);

    SpawnBeamEffect(Start, bHit ? OutHit.Location : End);
    SpawnTracerEffect(Start, bHit ? OutHit.Location : End);

    return bHit;
}
