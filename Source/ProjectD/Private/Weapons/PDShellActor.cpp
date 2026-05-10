// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/PDShellActor.h"
#include "Components/StaticMeshComponent.h"

APDShellActor::APDShellActor()
{
    PrimaryActorTick.bCanEverTick = false;

    ShellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShellMesh"));
    SetRootComponent(ShellMesh);

    ShellMesh->SetSimulatePhysics(true);
    ShellMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
    // 탄피가 캐릭터/무기에 충돌하지 않도록
    ShellMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    ShellMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

void APDShellActor::BeginPlay()
{
    Super::BeginPlay();
    SetLifeSpan(LifeTime);
}

void APDShellActor::Launch(const FVector& InitialVelocity)
{
    if (!ShellMesh) return;

    ShellMesh->SetPhysicsLinearVelocity(InitialVelocity);

    // 탄피가 튀면서 자연스럽게 굴러가도록 랜덤 각속도
    ShellMesh->SetPhysicsAngularVelocityInDegrees(FVector(
        FMath::RandRange(-AngularVelocityRange, AngularVelocityRange),
        FMath::RandRange(-AngularVelocityRange, AngularVelocityRange),
        FMath::RandRange(-AngularVelocityRange * 0.5f, AngularVelocityRange * 0.5f)
    ));
}
