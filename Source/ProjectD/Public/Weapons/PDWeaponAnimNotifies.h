// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PDWeaponAnimNotifies.generated.h"

// 발사 시 약실에서 탄피 배출
//   라이플: 발사 직후 / 스나이퍼: 노리쇠 후퇴 구간 / 샷건: 발사 직후
UCLASS(meta = (DisplayName = "Weapon: Eject Shell"))
class PROJECTD_API UPDAnimNotify_EjectShell : public UAnimNotify
{
    GENERATED_BODY()
public:
    virtual FString GetNotifyName_Implementation() const override
    {
        return TEXT("Eject Shell");
    }

    virtual void Notify(USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;
};


// 재장전 (라이플 / 스나이퍼)

// 탄창을 무기에서 제거 — MagMesh 숨기고 MagazineActor 스폰
UCLASS(meta = (DisplayName = "Weapon: Magazine Out"))
class PROJECTD_API UPDAnimNotify_MagOut : public UAnimNotify
{
    GENERATED_BODY()
public:
    virtual FString GetNotifyName_Implementation() const override
    {
        return TEXT("Magazine Out");
    }

    virtual void Notify(USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;
};

// 새 탄창 삽입 완료 — MagMesh 다시 표시
UCLASS(meta = (DisplayName = "Weapon: Magazine In"))
class PROJECTD_API UPDAnimNotify_MagIn : public UAnimNotify
{
    GENERATED_BODY()
public:
    virtual FString GetNotifyName_Implementation() const override
    {
        return TEXT("Magazine In");
    }

    virtual void Notify(USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;
};

// 노리쇠 후퇴 (장전 완료 직전 / 스나이퍼 발사 후 볼트백)
UCLASS(meta = (DisplayName = "Weapon: Bolt Pull"))
class PROJECTD_API UPDAnimNotify_BoltPull : public UAnimNotify
{
    GENERATED_BODY()
public:
    virtual FString GetNotifyName_Implementation() const override
    {
        return TEXT("Bolt Pull");
    }

    virtual void Notify(USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;
};

// 노리쇠 전진 (다음 탄 약실 삽입 완료)
UCLASS(meta = (DisplayName = "Weapon: Bolt Release"))
class PROJECTD_API UPDAnimNotify_BoltRelease : public UAnimNotify
{
    GENERATED_BODY()
public:
    virtual FString GetNotifyName_Implementation() const override
    {
        return TEXT("Bolt Release");
    }

    virtual void Notify(USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;
};


// ── 샷건 전용 ─────────────────────────────────────────────────

// 슬러그 탄 약실에 한 발 삽입 완료 — CurrentAmmo +1
UCLASS(meta = (DisplayName = "Weapon: Insert Shell (Shotgun)"))
class PROJECTD_API UPDAnimNotify_InsertShell : public UAnimNotify
{
    GENERATED_BODY()
public:
    virtual FString GetNotifyName_Implementation() const override
    {
        return TEXT("Insert Shell");
    }

    virtual void Notify(USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;
};
