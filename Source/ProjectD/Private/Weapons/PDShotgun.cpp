#include "Weapons/PDShotgun.h"
#include "DrawDebugHelpers.h"

APDShotgun::APDShotgun()
{
    WeaponType = EWeaponType::Shotgun;

    LevelStats.Add({ 15.f, 0.80f,  800.f, 6, 2.5f, 0.80f }); // Lv1
    LevelStats.Add({ 20.f, 0.75f,  900.f, 8, 2.2f, 0.83f }); // Lv2
    LevelStats.Add({ 28.f, 0.70f, 1000.f, 8, 2.0f, 0.87f }); // Lv3
}

void APDShotgun::Fire_Implementation()
{
    if (!CanFire()) return;

    TArray<FHitResult> Hits;
    PerformPelletTraces(Hits);

    TSet<AActor*> HitActors;
    for (const FHitResult& Hit : Hits)
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && !HitActors.Contains(HitActor))
        {
            HitActors.Add(HitActor);
            ApplyDamage(HitActor, GetCurrentStats().Damage);
        }
    }

    PostFire();

    // 발사 직후 탄피 배출
    EjectShell();
}

void APDShotgun::Reload_Implementation()
{
    if (bIsReloading) return;
    if (CurrentAmmo >= GetCurrentStats().MaxAmmo) return;

    bIsReloading = true;
    ShellsToLoad = GetCurrentStats().MaxAmmo - CurrentAmmo;
    ShellsInserted = 0;

    LoadNextShell();
}

void APDShotgun::OnShellInserted_Implementation()
{
    ++ShellsInserted;
    // 실시간으로 탄약 UI에 반영
    ++CurrentAmmo;
    CurrentAmmo = FMath::Min(CurrentAmmo, GetCurrentStats().MaxAmmo);
}

void APDShotgun::LoadNextShell()
{
    if (ShellsInserted >= ShellsToLoad)
    {
        // 모든 탄 삽입 완료 → 펌프 동작 후 재장전 완료
        if (PumpMontage)
        {
            PlayWeaponMontage(PumpMontage);
            UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
            if (AnimInst)
            {
                FOnMontageEnded EndDelegate;
                EndDelegate.BindUObject(this, &APDShotgun::OnShellInsertMontageEnded);
                AnimInst->Montage_SetEndDelegate(EndDelegate, PumpMontage);
            }
        }
        else
        {
            FinishReload();
        }
        return;
    }

    if (ShellInsertMontage)
    {
        PlayWeaponMontage(ShellInsertMontage);

        UAnimInstance* AnimInst = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
        if (AnimInst)
        {
            FOnMontageEnded EndDelegate;
            EndDelegate.BindUObject(this, &APDShotgun::OnShellInsertMontageEnded);
            AnimInst->Montage_SetEndDelegate(EndDelegate, ShellInsertMontage);
        }
    }
    else
    {
        // 몽타주 없을 때 폴백: 타이머로 한 발씩
        FTimerHandle TempTimer;
        GetWorldTimerManager().SetTimer(
            TempTimer, this,
            &APDShotgun::LoadNextShell,
            ShellInsertTime, false);
    }
}

void APDShotgun::InterruptReloadAndFire()
{
    if (!bIsReloading || CurrentAmmo <= 0) return;

    // 진행 중인 몽타주 중단
    StopWeaponMontage(ShellInsertMontage);
    StopWeaponMontage(PumpMontage);
    bIsReloading = false;

    Fire();
}

void APDShotgun::OnShellInsertMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (bInterrupted)
    {
        // 도중 중단 (발사 등) — 장전 상태 정리
        bIsReloading = false;
        return;
    }

    if (Montage == PumpMontage)
    {
        FinishReload();
        return;
    }

    // 다음 탄 삽입
    LoadNextShell();
}

int32 APDShotgun::GetCurrentPelletCount() const
{
    int32 Idx = FMath::Clamp(CurrentLevel - 1, 0, PelletCountPerLevel.Num() - 1);
    return PelletCountPerLevel[Idx];
}

void APDShotgun::PerformPelletTraces(TArray<FHitResult>& OutHits)
{
    AActor* WeaponOwnerActor = GetWeaponOwner();
    if (!WeaponOwnerActor) return;
 
    APlayerController* PC = Cast<APlayerController>(WeaponOwnerActor->GetInstigatorController());
 
    FVector Start = WeaponMesh->DoesSocketExist(MuzzleSocketName)
        ? WeaponMesh->GetSocketLocation(MuzzleSocketName)
        : WeaponOwnerActor->GetActorLocation();
 
    FVector Forward = WeaponOwnerActor->GetActorForwardVector();
 
    if (PC)
    {
        // 1순위: 커서가 Pawn 위 → 부위 조준
        FHitResult PawnHit;
        if (PC->GetHitResultUnderCursorForObjects(
            { UEngineTypes::ConvertToObjectType(ECC_Pawn) }, true, PawnHit)
            && PawnHit.GetActor() && PawnHit.GetActor() != WeaponOwnerActor)
        {
            FVector Dir = PawnHit.Location - Start;
            if (!Dir.IsNearlyZero()) Forward = Dir.GetSafeNormal();
        }
        // 2순위: 지면 커서
        else
        {
            FHitResult CursorHit;
            if (PC->GetHitResultUnderCursor(ECC_Visibility, true, CursorHit))
            {
                FVector Dir = CursorHit.Location - Start;
                if (!Dir.IsNearlyZero()) Forward = Dir.GetSafeNormal();
            }
        }
    }
 
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(WeaponOwnerActor);
    Params.bTraceComplex = true;
 
    const float Range   = GetCurrentStats().Range;
    const int32 Pellets = GetCurrentPelletCount();
 
    for (int32 i = 0; i < Pellets; ++i)
    {
        FVector RandDir = FMath::VRandCone(Forward, FMath::DegreesToRadians(SpreadAngle));
        FVector End     = Start + RandDir * Range;
        FHitResult Hit;
        if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params))
        {
            OutHits.Add(Hit);
            DrawDebugLine(GetWorld(), Start, Hit.Location, FColor::Red, false, 1.f, 0, 1.f);
        }
        else DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.f, 0, 1.f);
    }
}
