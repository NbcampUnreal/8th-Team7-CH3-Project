#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/PDEnemyBase.h"
#include "PDBipedEnemy.generated.h"

class UPDStatComponent;
class UPDCombatComponent;

/**
 * 이족 보행 적 추상 베이스.
 *  - StatComponent + CombatComponent 의무 보유.
 *  - bUseStamina=true 로 Stamina 시스템 활성.
 *  - GetStaminaStatus 를 StatComponent 기반으로 오버라이드.
 *
 * "Biped" 카테고리를 클래스 계층으로 명시 → 4족/비행 적이 추가될 때
 * Stamina/이족 가정이 새지 않도록 격리.
 */
UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDBipedEnemy : public APDEnemyBase
{
	GENERATED_BODY()

public:
	APDBipedEnemy();

	virtual EPDStaminaStatus GetStaminaStatus_Implementation() const override;

	UFUNCTION(BlueprintPure, Category = "PD|Stat")
	FORCEINLINE UPDStatComponent* GetStatComponent() const { return StatComponent; }

	UFUNCTION(BlueprintPure, Category = "PD|Combat")
	FORCEINLINE UPDCombatComponent* GetCombatComponent() const { return CombatComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Stat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDStatComponent> StatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDCombatComponent> CombatComponent;
};
