// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/PDWeaponAnimNotifies.h"
#include "Weapons/PDWeaponBase.h"
#include "Components/SkeletalMeshComponent.h"

// MeshComp 소유 무기 꺼내기
// MeshComp->GetOwner()는 WeaponMesh의 Owner = APDWeaponBase
static APDWeaponBase* GetWeapon(USkeletalMeshComponent* MeshComp)
{
    return MeshComp ? Cast<APDWeaponBase>(MeshComp->GetOwner()) : nullptr;
}


// 탄피 배출
void UPDAnimNotify_EjectShell::Notify(USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);
    if (APDWeaponBase* Weapon = GetWeapon(MeshComp))
        Weapon->EjectShell();
}


// 탄창 제거
void UPDAnimNotify_MagOut::Notify(USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);
    if (APDWeaponBase* Weapon = GetWeapon(MeshComp))
        Weapon->DropMagazine();
}


// 탄창 삽입
void UPDAnimNotify_MagIn::Notify(USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);
    if (APDWeaponBase* Weapon = GetWeapon(MeshComp))
        Weapon->AttachNewMagazine();
}


// 노리쇠 후퇴
void UPDAnimNotify_BoltPull::Notify(USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);
    if (APDWeaponBase* Weapon = GetWeapon(MeshComp))
        Weapon->OnBoltPulled();
}


// 노리쇠 전진
void UPDAnimNotify_BoltRelease::Notify(USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);
    if (APDWeaponBase* Weapon = GetWeapon(MeshComp))
        Weapon->OnBoltReleased();
}


// ── 샷건 탄 한 발 삽입 ───────────────────────────────────────
void UPDAnimNotify_InsertShell::Notify(USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);
    if (APDWeaponBase* Weapon = GetWeapon(MeshComp))
        Weapon->OnShellInserted();
}
