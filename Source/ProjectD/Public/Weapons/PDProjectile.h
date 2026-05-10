// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Type/Types.h"
#include "Interfaces/PDDamageable.h"
#include "PDProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class PROJECTD_API APDProjectile : public AActor
{
	GENERATED_BODY()
	
public:
    APDProjectile();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Projectile|Component")
    TObjectPtr<USphereComponent> CollisionComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Projectile|Component")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Projectile|Config")
    float InitialSpeed = 8000.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Projectile|Config")
    float LifeSpan = 5.f;

    // 런타임 주입값 
    float Damage = 0.f;
    bool  bCanPenetrate = false;
    int32 MaxPenetrationCount = 3;
    int32 PenetrationCount = 0;

    UPROPERTY()
    TWeakObjectPtr<AActor> WeaponOwner;

public:
    void InitProjectile(float InDamage, AActor* InOwner, bool bPenetrate);

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse,
        const FHitResult& Hit);
};
