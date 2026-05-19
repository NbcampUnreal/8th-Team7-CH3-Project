#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Type/Types.h"
#include "PDGameInstance.generated.h"

UCLASS()
class PROJECTD_API UPDGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	
	UFUNCTION(BlueprintCallable, Category = "PD|Save")
	void SetPlayerData(const FPDPlayerData& InData);
	
	UFUNCTION(BlueprintCallable, Category = "PD|Save")
	FPDPlayerData GetPlayerData() const { return PlayerData; }

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void SetStashItems(const TArray<FPDInventorySlot>& InStashItems);

	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	const TArray<FPDInventorySlot>& GetStashItems() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void SetStashUpgradeLevel(int32 InUpgradeLevel);

	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	int32 GetStashUpgradeLevel() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void SetTraderReputation(int32 InLevel, int32 InExp);

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void SetTraderReputationExp(int32 InExp);

	UFUNCTION(BlueprintCallable, Category = "PD|Market")
	void SetTraderReputationLevel(int32 InLevel);

	UFUNCTION(BlueprintPure, Category = "PD|Market")
	int32 GetTraderReputationExp() const;

	UFUNCTION(BlueprintPure, Category = "PD|Market")
	int32 GetTraderReputationLevel() const;
	
	// ── 레이드 로드아웃 ──────────────────────────────────────────────────────
	/**
	 * 허브 로드아웃 확정 화면에서 호출.
	 * 선택한 아이템/골드를 RaidLoadout 에 기록하고,
	 * StashItems 에서는 해당 항목을 미리 차감해 SaveToDisk() 까지 수행.
	 * (사망해도 스태시에는 이미 없는 상태로 저장됨)
	 */
	UFUNCTION(BlueprintCallable, Category="PD|Raid")
	void ConfirmRaidLoadout(const TArray<FPDInventorySlot>& InLoadout, int32 InGold);

	UFUNCTION(BlueprintPure, Category="PD|Raid")
	const TArray<FPDInventorySlot>& GetRaidLoadout() const { return PlayerData.RaidLoadout; }

	UFUNCTION(BlueprintPure, Category="PD|Raid")
	int32 GetRaidGold() const { return PlayerData.RaidGold; }

	/** StartRaid() 내부에서 인벤토리 이전 완료 후 호출. */
	UFUNCTION(BlueprintCallable, Category="PD|Raid")
	void ClearRaidLoadout();
	// ─────────────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "PD|Save")
	void SaveToDisk();

	UFUNCTION(BlueprintCallable, Category = "PD|Save")
	void LoadFromDisk();

	UFUNCTION(BlueprintCallable, Category = "PD|Levels")
	void TravelToLevel(TSoftObjectPtr<UWorld> Level, bool bMarkBaseResetPending);

	UFUNCTION(BlueprintPure, Category = "PD|Levels")
	TSoftObjectPtr<UWorld> GetBaseLevel() const { return BaseLevel; }

	UFUNCTION(BlueprintCallable, Category = "PD|Levels")
	bool ConsumePendingResetToBase();

protected:
	UPROPERTY()
	FPDPlayerData PlayerData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Levels")
	TSoftObjectPtr<UWorld> BaseLevel;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Levels")
	bool bPendingResetToBase = false;
};
