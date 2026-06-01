#include "Weapons/PDSniper.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Core/PDPlayerController.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

APDSniper::APDSniper()
{
    WeaponType = EWeaponType::Sniper;

    DefaultFOV = 90.f;
    ZoomedFOV  = 40.f;

    LevelStats.Add({ 80.f,  1.5f,  8000.f, 5, 3.0f, 1.0f });
    LevelStats.Add({ 120.f, 1.3f, 10000.f, 5, 2.7f, 1.0f });
    LevelStats.Add({ 180.f, 1.2f, 15000.f, 5, 2.5f, 1.0f });
}

void APDSniper::Fire_Implementation()
{
    if (!HasAuthority())
    {
        return;
    }
    if (!CanFire())
    {
        return;
    }
    if (!ProjectileClass)
    {
        return;
    }

    FVector MuzzleLoc = FVector::ZeroVector;
    FVector AimDirection = FVector::ForwardVector;
    FVector TraceEnd = FVector::ZeroVector;
    if (!BuildAimShot(MuzzleLoc, AimDirection, TraceEnd))
    {
        return;
    }

    ExecuteFireCue(MuzzleLoc, TraceEnd);

    SpawnProjectile(CanPenetrate(), MuzzleLoc, AimDirection);

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

    PostFire();
}

void APDSniper::OnBoltActionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{

}

void APDSniper::ToggleZoom()
{
    bIsZoomed = !bIsZoomed;

    APlayerController* PC = nullptr;
    if (APawn* OwnerPawn = Cast<APawn>(GetWeaponOwner()))
        PC = Cast<APlayerController>(OwnerPawn->GetController());

    if (PC && PC->PlayerCameraManager)
        PC->PlayerCameraManager->SetFOV(bIsZoomed ? ZoomedFOV : DefaultFOV);

    OnScopeToggled.Broadcast(bIsZoomed);
}

void APDSniper::SpawnProjectile(bool bPenetrate, const FVector& Start, const FVector& AimDirection)
{
    AActor* WeaponOwnerActor = GetWeaponOwner();
    if (!WeaponOwnerActor)
    {
        return;
    }

    const FRotator SpawnRot = AimDirection.Rotation();
    const FTransform SpawnTransform(SpawnRot, Start);
    APDProjectile* Projectile = GetWorld()->SpawnActorDeferred<APDProjectile>(
        ProjectileClass,
        SpawnTransform,
        this,
        Cast<APawn>(WeaponOwnerActor),
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    if (Projectile)
    {
        Projectile->InitProjectile(GetCurrentStats().Damage, WeaponOwnerActor, bPenetrate, AimDirection);
        Projectile->FinishSpawning(SpawnTransform);
    }
}

bool APDSniper::BuildAimShot(FVector& OutStart, FVector& OutDirection, FVector& OutTraceEnd) const
{
    AActor* WeaponOwnerActor = GetWeaponOwner();
    if (!WeaponOwnerActor)
    {
        return false;
    }
    if (!WeaponMesh)
    {
        return false;
    }

    OutStart = WeaponMesh->DoesSocketExist(MuzzleSocketName)
        ? WeaponMesh->GetSocketLocation(MuzzleSocketName)
        : WeaponOwnerActor->GetActorLocation();

    OutDirection = GetAimDirectionFromOwner(OutStart);
    if (OutDirection.IsNearlyZero())
    {
        OutDirection = WeaponOwnerActor->GetActorForwardVector();
    }

    float TraceLength = GetCurrentStats().Range;
    FVector AimLocation = FVector::ZeroVector;
    if (const APawn* OwnerPawn = Cast<APawn>(WeaponOwnerActor))
    {
        if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
        {
            if (const APDPlayerController* PDPC = Cast<APDPlayerController>(PC);
                PDPC && PDPC->GetCachedAimWorldLocation(AimLocation))
            {
                TraceLength = FVector::Dist(OutStart, AimLocation);
            }
            else
            {
                FHitResult CursorHit;
                if (PC->GetHitResultUnderCursor(ECC_Visibility, true, CursorHit))
                {
                    TraceLength = FVector::Dist(OutStart, CursorHit.Location);
                    AimLocation = CursorHit.Location;
                }
            }
        }
    }

    OutDirection = OutDirection.GetSafeNormal();
    OutTraceEnd = OutStart + OutDirection * TraceLength;
    return true;
}

bool APDSniper::CanPenetrate() const
{
    int32 Idx = FMath::Clamp(CurrentLevel - 1, 0, PenetrationPerLevel.Num() - 1);
    return PenetrationPerLevel[Idx];
}
