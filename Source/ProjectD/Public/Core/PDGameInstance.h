#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Type/Types.h"
#include "PDGameInstance.generated.h"

class APlayerController;
class UPDSessionService;
class UPDInventoryComponent;

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

	UFUNCTION(BlueprintPure, Category = "PD|Save")
	FString GetSaveKeyForController(const APlayerController* PlayerController) const;

	UFUNCTION(BlueprintCallable, Category = "PD|Save")
	FPDPlayerData LoadPlayerDataFromDisk(const FString& SaveKey, bool bUseLegacyFallback = false) const;

	UFUNCTION(BlueprintCallable, Category = "PD|Save")
	void SavePlayerDataToDisk(const FString& SaveKey, const FPDPlayerData& InData) const;

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void SetStashItems(const TArray<FPDInventorySlot>& InStashItems);

	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	const TArray<FPDInventorySlot>& GetStashItems() const;

	// 공용 스태시에 슬롯 배열 병합 + 골드 가산. 서버 권한 전용(호스트 GI 가 권위 소스).
	// 컴팩트 리스트로 유지 → 스태시 컴포넌트 InitializeStash 가 그리드로 패딩.
	void MergeSlotsIntoStash(const TArray<FPDInventorySlot>& Slots, int32 GoldToAdd);

	UFUNCTION(BlueprintCallable, Category = "PD|Stash")
	void SetStashUpgradeLevel(int32 InUpgradeLevel);

	UFUNCTION(BlueprintPure, Category = "PD|Stash")
	int32 GetStashUpgradeLevel() const;

	// === SecureContainer (Source에서 이식) ===
	UFUNCTION(BlueprintCallable, Category = "PD|SecureContainer")
	void SetSecureContainerItems(const TArray<FPDInventorySlot>& InSecureContainerItems);

	const TArray<FPDInventorySlot>& GetSecureContainerItems() const;

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



	UFUNCTION(BlueprintCallable, Category="PD|Raid")
	void ConfirmRaidLoadout(const TArray<FPDInventorySlot>& InLoadout, int32 InGold);

	UFUNCTION(BlueprintPure, Category="PD|Raid")
	const TArray<FPDInventorySlot>& GetRaidLoadout() const { return PlayerData.RaidLoadout; }

	UFUNCTION(BlueprintPure, Category="PD|Raid")
	int32 GetRaidGold() const { return PlayerData.RaidGold; }


	UFUNCTION(BlueprintCallable, Category="PD|Raid")
	void ClearRaidLoadout();


	UFUNCTION(BlueprintCallable, Category = "PD|Save")
	void SaveToDisk();

	UFUNCTION(BlueprintCallable, Category = "PD|Save")
	void LoadFromDisk();

	UFUNCTION(BlueprintCallable, Category = "PD|Levels")
	void TravelToLevel(TSoftObjectPtr<UWorld> Level, bool bMarkBaseResetPending);

	UFUNCTION(BlueprintPure, Category = "PD|Levels")
	TSoftObjectPtr<UWorld> GetBaseLevel() const { return BaseLevel; }

	UFUNCTION(BlueprintPure, Category = "PD|Levels")
	TSoftObjectPtr<UWorld> GetLobbyLevel() const { return LobbyLevel; }

	UFUNCTION(BlueprintPure, Category = "PD|Levels")
	TSoftObjectPtr<UWorld> GetStartupLevel() const { return StartupLevel; }

	UFUNCTION(BlueprintCallable, Category = "PD|Levels")
	bool ConsumePendingResetToBase();

	UFUNCTION(BlueprintPure, Category = "PD|Session")
	UPDSessionService* GetSessionService() const { return ActiveSessionService; }

	/** Standalone/빌드에서 호스팅·참가로 LobbyLevel에 (재)진입할 때, MainMenu를 건너뛰고
	 *  방 화면(RoomScreen)으로 바로 가도록 표시. SessionService 호출 직전에 set. */
	UFUNCTION(BlueprintCallable, Category = "PD|Session")
	void SetPendingRoomScreen() { bPendingRoomScreen = true; }

	/** LobbyPC BeginPlay에서 1회 소비. true면 RoomScreen, 아니면 MainMenu를 push. */
	UFUNCTION(BlueprintCallable, Category = "PD|Session")
	bool ConsumePendingRoomScreen();

protected:
	UPROPERTY()
	FPDPlayerData PlayerData;

	// SecureContainer의 원래 동작 보존: 레이드 간 메모리 조롤 (세이브 파일이 아닌 세션 수명)
	UPROPERTY(Transient)
	TArray<FPDInventorySlot> SecureContainerItems;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Levels")
	TSoftObjectPtr<UWorld> BaseLevel;

	/** 호스트가 listen 모드로 띄울 LobbyLevel 자산. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Levels")
	TSoftObjectPtr<UWorld> LobbyLevel;

	/** 방 나가기(Leave) 시 복귀할 메인 메뉴 레벨. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Levels")
	TSoftObjectPtr<UWorld> StartupLevel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Session")
	TSubclassOf<UPDSessionService> SessionServiceClass;

	UPROPERTY(Transient)
	TObjectPtr<UPDSessionService> ActiveSessionService;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Levels")
	bool bPendingResetToBase = false;

	bool bPendingRoomScreen = false;

private:
	FString MakePlayerSlotName(const FString& SaveKey) const;
	FString SanitizeSaveKey(const FString& SaveKey) const;
};
