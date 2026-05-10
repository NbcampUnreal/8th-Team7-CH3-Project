// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDMagazineActor.generated.h"

UCLASS(Blueprintable)
class PROJECTD_API APDMagazineActor : public AActor
{
	GENERATED_BODY()
	
public:
    APDMagazineActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Magazine")
    TObjectPtr<UStaticMeshComponent> MagMesh;

    // 무기 메시에서 탄창 메시를 복사해 쓸 수 있도록 런타임에 메시 세팅
    void InitFromWeapon(UStaticMesh* InMesh, UMaterialInterface* InMaterial = nullptr);

    // 탄창을 바닥으로 떨어뜨림
    void Drop();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category = "PD|Magazine")
    float LifeTime = 6.f;

    // 탄창이 떨어질 때 살짝 앞으로 튀는 속도
    UPROPERTY(EditDefaultsOnly, Category = "PD|Magazine")
    float DropImpulse = 60.f;
};