// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDShellActor.generated.h"

UCLASS(Blueprintable)
class PROJECTD_API APDShellActor : public AActor
{
	GENERATED_BODY()
	
public:
    APDShellActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Shell")
    TObjectPtr<UStaticMeshComponent> ShellMesh;

    // 스폰 직후 호출 — 초기 속도 + 랜덤 회전 부여
    void Launch(const FVector& InitialVelocity);

protected:
    virtual void BeginPlay() override;

    // BP에서 메시별로 조정 가능 (라이플탄피 / 샷건슬러그 / 스나이퍼탄피)
    UPROPERTY(EditDefaultsOnly, Category = "PD|Shell")
    float LifeTime = 4.f;

    UPROPERTY(EditDefaultsOnly, Category = "PD|Shell")
    float AngularVelocityRange = 400.f;
};
