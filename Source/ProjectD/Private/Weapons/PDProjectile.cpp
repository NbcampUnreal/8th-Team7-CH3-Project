


#include "Weapons/PDProjectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Interfaces/PDDamageable.h"
#include "Net/UnrealNetwork.h"

APDProjectile::APDProjectile()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    // 이동은 복제하지 않음 — 스폰 위치/회전은 스폰 정보로 전달되고, 방향은 LaunchVelocity 로 복제해
    // 클라가 결정적으로 시뮬레이션. (복제 이동 ↔ ProjectileMovement 충돌로 클라 방향 틀어짐 방지)
    SetReplicateMovement(false);

    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
    CollisionComp->InitSphereRadius(5.f);
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    CollisionComp->SetGenerateOverlapEvents(true);
    CollisionComp->SetNotifyRigidBodyCollision(true);
    SetRootComponent(CollisionComp);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bInitialVelocityInLocalSpace = false;
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->ProjectileGravityScale = 0.f;
    ProjectileMovement->InitialSpeed = InitialSpeed;
    ProjectileMovement->MaxSpeed = InitialSpeed;

    InitialLifeSpan = LifeSpan;
}

void APDProjectile::BeginPlay()
{
    Super::BeginPlay();
    CollisionComp->OnComponentHit.AddDynamic(this, &APDProjectile::OnHit);
    CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &APDProjectile::OnOverlap);
}

void APDProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    // 스폰 시 1회만 전달되면 충분(발사 후 방향 불변).
    DOREPLIFETIME_CONDITION(APDProjectile, LaunchVelocity, COND_InitialOnly);
}

// 클라: 복제된 발사 속도로 PMC 속도·회전을 맞춰 서버와 동일 방향으로 비행.
void APDProjectile::OnRep_LaunchVelocity()
{
    if (ProjectileMovement && !LaunchVelocity.IsNearlyZero())
    {
        ProjectileMovement->Velocity = LaunchVelocity;
        ProjectileMovement->UpdateComponentVelocity();
        SetActorRotation(LaunchVelocity.Rotation());
    }
}

void APDProjectile::InitProjectile(float InDamage, AActor* InOwner, bool bPenetrate, const FVector& InitialDirection)
{
    Damage = InDamage;
    WeaponOwner = InOwner;
    bCanPenetrate = bPenetrate;

    if (CollisionComp)
    {
        if (InOwner)
        {
            CollisionComp->IgnoreActorWhenMoving(InOwner, true);
        }
        if (AActor* WeaponActor = GetOwner())
        {
            CollisionComp->IgnoreActorWhenMoving(WeaponActor, true);
        }
    }

    if (ProjectileMovement && !InitialDirection.IsNearlyZero())
    {
        const float Speed = ProjectileMovement->InitialSpeed > 0.f ? ProjectileMovement->InitialSpeed : InitialSpeed;
        ProjectileMovement->bInitialVelocityInLocalSpace = false;
        ProjectileMovement->ProjectileGravityScale = 0.f;
        ProjectileMovement->MaxSpeed = FMath::Max(ProjectileMovement->MaxSpeed, Speed);
        ProjectileMovement->Velocity = InitialDirection.GetSafeNormal() * Speed;
        ProjectileMovement->UpdateComponentVelocity();

        // 클라 복제용 — 스폰 직후(첫 복제 전) 설정되므로 초기 번들에 포함됨.
        LaunchVelocity = ProjectileMovement->Velocity;
    }
}

void APDProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse,
    const FHitResult& Hit)
{
    HandleProjectileHit(OtherActor, Hit);
}

void APDProjectile::OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    HandleProjectileHit(OtherActor, SweepResult);
}

bool APDProjectile::HandleProjectileHit(AActor* OtherActor, const FHitResult& Hit)
{
    if (!HasAuthority())
    {
        return false;
    }
    if (!OtherActor || OtherActor == this || OtherActor == WeaponOwner.Get() || OtherActor == GetOwner())
    {
        return false;
    }

    if (!OtherActor->Implements<UPDDamageable>())
    {
        Destroy();
        return false;
    }

    FPDDamageInfo DamageInfo;
    DamageInfo.BaseDamage = Damage;
    DamageInfo.Instigator = WeaponOwner;
    DamageInfo.DamageTypeClass = nullptr;
    DamageInfo.HitResult = Hit;

    IPDDamageable::Execute_ApplyDamage(OtherActor, DamageInfo);

    if (bCanPenetrate && PenetrationCount < MaxPenetrationCount)
    {
        ++PenetrationCount;
        if (CollisionComp)
        {
            CollisionComp->IgnoreActorWhenMoving(OtherActor, true);
        }
        return true;
    }

    Destroy();
    return true;
}
