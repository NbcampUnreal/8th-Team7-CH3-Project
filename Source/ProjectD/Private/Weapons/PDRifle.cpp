#include "Weapons/PDRifle.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Core/PDPlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

namespace
{
bool RefineCharacterHitToMesh(const FHitResult& SourceHit, const FVector& Start, const FVector& End,
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

APDRifle::APDRifle()
{
    WeaponType = EWeaponType::Rifle;
    bFullAuto  = true;

    LevelStats.Add({ 20.f, 0.12f, 1500.f, 30, 2.0f, 0.90f });
    LevelStats.Add({ 28.f, 0.10f, 1700.f, 35, 1.8f, 0.93f });
    LevelStats.Add({ 38.f, 0.08f, 2000.f, 40, 1.5f, 0.96f });
}

void APDRifle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(APDRifle, FireMode);
}

void APDRifle::Fire_Implementation()
{
    if (!HasAuthority()) return;
    if (!CanFire())
    {
        return;
    }

    FHitResult Hit;
    FVector TraceEnd;
    const bool bHit = PerformLineTrace(Hit, TraceEnd);

    if (bHit)
        ApplyDamage(Hit.GetActor(), GetCurrentStats().Damage, Hit);

    const FVector MuzzleLoc = WeaponMesh->DoesSocketExist(MuzzleSocketName)
        ? WeaponMesh->GetSocketLocation(MuzzleSocketName)
        : GetActorLocation();


    ExecuteFireCue(MuzzleLoc, bHit ? Hit.ImpactPoint : TraceEnd);


    if (bHit)
        ExecuteImpactCue(Hit);

    PlayWeaponMontage(FireMontage);
    PostFire();
}

void APDRifle::ToggleFireMode()
{
    if (!HasAuthority()) return;

    FireMode = (FireMode == EFireMode::Auto)
        ? EFireMode::Single : EFireMode::Auto;

    OnFireModeChanged.Broadcast(FireMode);
}

void APDRifle::OnRep_FireMode()
{
    OnFireModeChanged.Broadcast(FireMode);
}

bool APDRifle::PerformLineTrace(FHitResult& OutHit, FVector& OutTraceEnd)
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

    const float TotalSpread = FMath::DegreesToRadians((1.f - Stats.Accuracy) * 5.f);
    const FVector ShootDir  = FMath::VRandCone(AimDir, TotalSpread);
    OutTraceEnd              = Start + ShootDir * TraceLength;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.AddIgnoredActor(WeaponOwnerActor);
    QueryParams.bTraceComplex = true;

    const bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, OutTraceEnd, ECC_Pawn, QueryParams);
    if (bHit)
    {
        FHitResult MeshHit;
        if (RefineCharacterHitToMesh(OutHit, Start, OutTraceEnd, QueryParams, MeshHit))
        {
            OutHit = MeshHit;
        }
    }
    if (bHit) OutTraceEnd = OutHit.ImpactPoint;

    return bHit;
}
