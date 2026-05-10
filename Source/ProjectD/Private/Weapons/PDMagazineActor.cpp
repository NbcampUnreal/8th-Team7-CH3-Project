// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/PDMagazineActor.h"
#include "Components/StaticMeshComponent.h"

APDMagazineActor::APDMagazineActor()
{
    PrimaryActorTick.bCanEverTick = false;

    MagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagMesh"));
    SetRootComponent(MagMesh);

    MagMesh->SetSimulatePhysics(true);
    MagMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
    MagMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    MagMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

void APDMagazineActor::BeginPlay()
{
    Super::BeginPlay();
    SetLifeSpan(LifeTime);
}

void APDMagazineActor::InitFromWeapon(UStaticMesh* InMesh, UMaterialInterface* InMaterial)
{
    if (!MagMesh) return;
    if (InMesh)     MagMesh->SetStaticMesh(InMesh);
    if (InMaterial) MagMesh->SetMaterial(0, InMaterial);
}

void APDMagazineActor::Drop()
{
    if (!MagMesh) return;

    // 무기 앞쪽 + 아래로 살짝 튀어나오게
    FVector DropDir = GetActorForwardVector() * DropImpulse
        + FVector(0.f, 0.f, -DropImpulse * 0.5f);
    MagMesh->SetPhysicsLinearVelocity(DropDir);
}