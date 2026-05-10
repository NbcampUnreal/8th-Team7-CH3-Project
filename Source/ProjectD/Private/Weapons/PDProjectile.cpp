// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/PDProjectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Interfaces/PDDamageable.h"

APDProjectile::APDProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
    CollisionComp->InitSphereRadius(5.f);
    CollisionComp->SetCollisionProfileName(TEXT("Projectile"));
    CollisionComp->SetNotifyRigidBodyCollision(true);
    SetRootComponent(CollisionComp);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->InitialSpeed = InitialSpeed;
    ProjectileMovement->MaxSpeed = InitialSpeed;

    InitialLifeSpan = LifeSpan;
}

void APDProjectile::BeginPlay()
{
    Super::BeginPlay();
    CollisionComp->OnComponentHit.AddDynamic(this, &APDProjectile::OnHit);
}

void APDProjectile::InitProjectile(float InDamage, AActor* InOwner, bool bPenetrate)
{
    Damage = InDamage;
    WeaponOwner = InOwner;
    bCanPenetrate = bPenetrate;
}

void APDProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse,
    const FHitResult& Hit)
{
    if (!OtherActor || OtherActor == this || OtherActor == WeaponOwner.Get()) return;

    // 1. IPDDamageable 인터페이스 있는지 체크
    if (!OtherActor->Implements<UPDDamageable>()) return;

    // 2. 이미 죽은 대상 체크
    if (!IPDDamageable::Execute_IsAlive(OtherActor)) return;

    // 3. 데미지 적용
    FPDDamageInfo DamageInfo;
    DamageInfo.BaseDamage = Damage;
    DamageInfo.Instigator = WeaponOwner;
    DamageInfo.DamageTypeClass = nullptr;
    DamageInfo.HitResult = Hit;

    IPDDamageable::Execute_ApplyDamage(OtherActor, DamageInfo);

    // 4. 관통 처리
    if (bCanPenetrate && PenetrationCount < MaxPenetrationCount)
        ++PenetrationCount;
    else
        Destroy();
}