#include "Weapons/PDShotgun.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Core/PDPlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

namespace
{
bool RefineShotgunCharacterHitToMesh(const FHitResult& SourceHit, const FVector& Start, const FVector& End,
    const FCollisionQueryParams& QueryParams, FHitResult& OutHit)
{
    ACharacter* HitCharacter = Cast<ACharacter>(SourceHit.GetActor());
    if (!HitCharacter)
    {
        return false;
    }

    USkeletalMeshComponent* Mesh = HitCharacter->GetMesh();
    if (!Mesh || SourceHit.GetComponent() == Mesh)
    {
        return false;
    }

    FCollisionQueryParams MeshQueryParams = QueryParams;
    MeshQueryParams.bTraceComplex = false;

    FHitResult MeshHit;
    if (!Mesh->LineTraceComponent(MeshHit, Start, End, MeshQueryParams))
    {
        FVector ClosestBoneLocation = FVector::ZeroVector;
        const FName ClosestBoneName = Mesh->FindClosestBone(SourceHit.ImpactPoint, &ClosestBoneLocation, 0.f, false);
        if (ClosestBoneName.IsNone())
        {
            return false;
        }

        OutHit = SourceHit;
        OutHit.Component = Mesh;
        OutHit.BoneName = ClosestBoneName;
        OutHit.MyBoneName = ClosestBoneName;
        OutHit.TraceStart = Start;
        OutHit.TraceEnd = End;
        return true;
    }

    OutHit = SourceHit;
    OutHit.Component = Mesh;
    OutHit.BoneName = MeshHit.BoneName;
    OutHit.MyBoneName = MeshHit.MyBoneName;
    OutHit.ImpactPoint = MeshHit.ImpactPoint;
    OutHit.Location = MeshHit.Location;
    OutHit.ImpactNormal = MeshHit.ImpactNormal;
    OutHit.Normal = MeshHit.Normal;
    OutHit.TraceStart = Start;
    OutHit.TraceEnd = End;
    OutHit.Distance = MeshHit.Distance;
    OutHit.FaceIndex = MeshHit.FaceIndex;
    OutHit.PhysMaterial = MeshHit.PhysMaterial;
    return true;
}
}

APDShotgun::APDShotgun()
{
    WeaponType = EWeaponType::Shotgun;

    LevelStats.Add({ 15.f, 0.80f,  800.f, 6, 2.5f, 0.80f });
    LevelStats.Add({ 20.f, 0.75f,  900.f, 8, 2.2f, 0.83f });
    LevelStats.Add({ 28.f, 0.70f, 1000.f, 8, 2.0f, 0.87f });
}

void APDShotgun::Fire_Implementation()
{
    if (!HasAuthority()) return;
    if (!CanFire()) return;

    const FVector MuzzleLoc = WeaponMesh->DoesSocketExist(MuzzleSocketName)
        ? WeaponMesh->GetSocketLocation(MuzzleSocketName)
        : GetActorLocation();


    ExecuteFireCue(MuzzleLoc, FVector::ZeroVector);


    TArray<FHitResult> Hits;
    PerformPelletTraces(Hits);


    bool bImpactCueSent = false;
    const int32 PelletCount = FMath::Max(1, GetCurrentPelletCount());
    const float DamagePerPellet = GetCurrentStats().Damage / static_cast<float>(PelletCount);
    for (const FHitResult& Hit : Hits)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor) continue;

        ApplyDamage(HitActor, DamagePerPellet, Hit);

        if (!bImpactCueSent)
        {
            ExecuteImpactCue(Hit);
            bImpactCueSent = true;
        }
    }

    PlayWeaponMontage(FireMontage);
    PostFire();
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

    APlayerController* PC = nullptr;
    if (APawn* OwnerPawn = Cast<APawn>(WeaponOwnerActor))
        PC = Cast<APlayerController>(OwnerPawn->GetController());

    FVector Start = WeaponMesh->DoesSocketExist(MuzzleSocketName)
        ? WeaponMesh->GetSocketLocation(MuzzleSocketName)
        : WeaponOwnerActor->GetActorLocation();

    FVector Forward = GetAimDirectionFromOwner(Start);

    float TraceLength = GetCurrentStats().Range;
    if (PC)
    {
        FVector AimLocation;
        if (const APDPlayerController* PDPC = Cast<APDPlayerController>(PC);
            PDPC && PDPC->GetCachedAimWorldLocation(AimLocation))
        {
            TraceLength = FVector::Dist(Start, AimLocation);
        }
        else
        {
            FHitResult CursorHit;
            if (PC->GetHitResultUnderCursor(ECC_Visibility, true, CursorHit))
            {
                TraceLength = FVector::Dist(Start, CursorHit.Location);
            }
        }
    }

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(WeaponOwnerActor);
    Params.bTraceComplex = true;

    const int32 Pellets = GetCurrentPelletCount();
    const float EffectiveSpreadAngle = SpreadAngle * FMath::Clamp(1.f - GetCurrentStats().Accuracy, 0.f, 1.f);

    for (int32 i = 0; i < Pellets; ++i)
    {
        FVector RandDir = FMath::VRandCone(Forward, FMath::DegreesToRadians(EffectiveSpreadAngle));
        FVector End     = Start + RandDir * TraceLength;

        FHitResult Hit;
        const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);
        if (bHit)
        {
            FHitResult MeshHit;
            if (RefineShotgunCharacterHitToMesh(Hit, Start, End, Params, MeshHit))
            {
                Hit = MeshHit;
            }
        }

        if (bHit) OutHits.Add(Hit);
    }

}
