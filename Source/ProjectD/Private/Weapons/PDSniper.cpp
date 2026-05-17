#include "Weapons/PDSniper.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

APDSniper::APDSniper()
{
    WeaponType = EWeaponType::Sniper;

    DefaultFOV = 90.f;
    ZoomedFOV  = 40.f;

    LevelStats.Add({ 80.f,  1.5f,  8000.f, 5, 3.0f, 1.0f }); // Lv1
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

    PlayFireEffects();
    SpawnCartridge();
    SpawnProjectile(CanPenetrate());

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
    // 볼트 액션 완료 후 처리 필요 시 여기서
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

void APDSniper::SpawnProjectile(bool bPenetrate)
{
    AActor* WeaponOwnerActor = GetWeaponOwner();
    if (!WeaponOwnerActor) return;

    FVector Start = WeaponMesh->DoesSocketExist(MuzzleSocketName)
        ? WeaponMesh->GetSocketLocation(MuzzleSocketName)
        : WeaponOwnerActor->GetActorLocation();

    FRotator SpawnRot = GetAimDirection().Rotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner       = this;
    SpawnParams.Instigator  = Cast<APawn>(WeaponOwnerActor);
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    APDProjectile* Projectile = GetWorld()->SpawnActor<APDProjectile>(
        ProjectileClass, Start, SpawnRot, SpawnParams);

    if (Projectile)
        Projectile->InitProjectile(GetCurrentStats().Damage, WeaponOwnerActor, bPenetrate);
}

FVector APDSniper::GetAimDirection() const
{
    AActor* WeaponOwnerActor = GetWeaponOwner();
    if (!WeaponOwnerActor) return FVector::ForwardVector;

    FVector Start = WeaponMesh->DoesSocketExist(MuzzleSocketName)
        ? WeaponMesh->GetSocketLocation(MuzzleSocketName)
        : WeaponOwnerActor->GetActorLocation();

    return GetAimDirectionFromOwner(Start);
}

bool APDSniper::CanPenetrate() const
{
    int32 Idx = FMath::Clamp(CurrentLevel - 1, 0, PenetrationPerLevel.Num() - 1);
    return PenetrationPerLevel[Idx];
}
