#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Enemy/Types/EnemyTypes.h"
#include "Enemy/Interfaces/PDCombatInterface.h"
#include "Interfaces/PDDetectable.h"
#include "PDEnemyBase.generated.h"

class APDItemBase;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDEnemyBase : public APDCharacterBase, public IPDCombatInterface, public IPDDetectable
{
	GENERATED_BODY()

public:
	APDEnemyBase();

	//~ Begin IPDCombatInterface
	virtual uint8 GetTeamID_Implementation() const override;
	virtual EPDStaminaStatus GetStaminaStatus_Implementation() const override;
	//~ End IPDCombatInterface

	UFUNCTION(BlueprintCallable, Category = "PD|AI")
	void SetEnemyState(EPDEnemyState NewState);

	UFUNCTION(BlueprintPure, Category = "PD|AI")
	FORCEINLINE EPDEnemyState GetEnemyState() const { return CurrentState; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|AI")
	EPDEnemyState CurrentState = EPDEnemyState::Idle;

	/** 디자이너 확장 hook. 상태 전환 직후 호출. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|AI")
	void OnEnemyStateChanged(EPDEnemyState NewState);

	/** 상태별 진입 hook. 자식 클래스에서 오버라이드 가능. */
	virtual void OnEnterState_Idle()   {}
	virtual void OnEnterState_Alert()  {}
	virtual void OnEnterState_Chase()  {}
	virtual void OnEnterState_Combat() {}
	virtual void OnEnterState_Dead();

	virtual void HandleDeath(AActor* Killer) override;
	virtual void OnVisionExposureChanged_Implementation(AActor* Observer, float Exposure) override;

	// ─── 사망 드랍 ─────────────────────────────────────────────
	/** 사망 시 굴릴 드랍 테이블. 디자이너 BP 디폴트에서 채움. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot")
	TArray<FPDLootEntry> LootTable;

	/** 시체/박스 등 상호작용 가능한 컨테이너. 비어있으면 시체 미스폰. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot")
	TSubclassOf<AActor> CorpseContainerClass;

	/** 드랍 위치 변동 반경 — 0 이면 정확히 사망 위치에 스폰. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot", meta = (ClampMin = "0.0"))
	float LootSpawnRadius = 50.f;

	/** LootTable을 굴려 아이템을 스폰. 호출 시점은 자식이 결정 가능 (기본은 OnEnterState_Dead). */
	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	virtual void DropLootOnDeath();

	/** 시체/박스 컨테이너 스폰. 비어있는 클래스면 즉시 return. */
	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	virtual void SpawnCorpseContainer();

	/** 디자이너가 BP에서 드랍 후 처리(VFX/사운드 등) 작성 가능. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Loot")
	void OnLootDropped(const TArray<AActor*>& SpawnedItems);
};
