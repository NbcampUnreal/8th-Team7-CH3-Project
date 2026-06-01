// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDWeaponComponent.generated.h"

class APDWeaponBase;

/**
 * UPDWeaponComponent
 *
 * 역할: 조준 방향 하나만 담당
 *
 * 플레이어: AimTarget 없음 → PlayerController 커서 방향
 * 적(Enemy): SetAimTarget()으로 설정한 액터 방향
 *
 * 무기 장착/발사/장전은 각자가 직접 관리
 *   - APDPlayerCharacter → WeaponSlots
 *   - APDSoldier         → EquippedWeapon
 */

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTD_API UPDWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    UPDWeaponComponent();

    // ── 조준 타겟 (Enemy AI가 BT Task에서 호출) ─────────────────────────
    UFUNCTION(BlueprintCallable, Category = "PD|WeaponComponent")
    void SetAimTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "PD|WeaponComponent")
    void ClearAimTarget();

    /**
     * 조준 방향 반환 — PDWeaponBase::GetAimDirectionFromOwner()에서 호출
     *   플레이어: PlayerController 커서 방향
     *   적:       AimTarget 방향
     *   폴백:     Owner 전방 벡터
     */
    FVector GetAimDirection(const FVector& StartLocation) const;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|WeaponComponent",
        meta = (AllowPrivateAccess = "true"))
    TWeakObjectPtr<AActor> AimTarget;
};
