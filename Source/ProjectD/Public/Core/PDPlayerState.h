#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Game/PDRaidStats.h"
#include "Type/Types.h"
#include "PDPlayerState.generated.h"

class APDPlayerCharacter;
class UPDEquipmentComponent;
class UPDInventoryComponent;
class UPDQuestComponent;
class UPDQuickSlotComponent;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDOnRaidParticipationChanged, bool, bIsExtracted, bool, bIsRaidDead);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDOnTravelReadyChanged, bool, bIsTravelReady);

UCLASS(Blueprintable)
class PROJECTD_API APDPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	APDPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// seamless travel 시 새 PlayerState 로 PersistentSaveId + PersistentData 보존.
	virtual void CopyProperties(APlayerState* PlayerState) override;

	UFUNCTION(BlueprintPure, Category="PD|PlayerState")
	UPDInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category="PD|PlayerState")
	UPDEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

	UFUNCTION(BlueprintPure, Category="PD|PlayerState")
	UPDQuickSlotComponent* GetQuickSlotComponent() const;

	UFUNCTION(BlueprintPure, Category="PD|PlayerState")
	UPDQuestComponent* GetQuestComponent() const { return QuestComponent; }

	UFUNCTION(BlueprintPure, Category="PD|PlayerState")
	APDPlayerCharacter* GetPDPlayerCharacter() const;

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Save")
	void InitializePersistentData(const FPDPlayerData& InData);

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Save")
	FPDPlayerData GetPersistentData() const { return PersistentData; }

	// 트래블 너머로 안정적인 저장 슬롯 키. PlayerName/PlayerId 는 seamless travel 시 바뀌므로 사용 불가.
	// 서버에서 최초 1회 할당 후 CopyProperties 로 트래블 너머 보존.
	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Save")
	const FString& GetPersistentSaveId() const { return PersistentSaveId; }

	// 서버 전용: 비어있으면 새 GUID 할당. 최초 로그인 시 1회.
	void EnsurePersistentSaveId();

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Market")
	int32 GetTraderReputationExp() const;

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Market")
	int32 GetTraderReputationLevel() const;

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Market")
	void SetTraderReputation(int32 InLevel, int32 InExp);

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Market")
	void AddTraderReputationExp(int32 Amount);

	const TArray<FPDInventorySlot>& GetRaidLoadout() const { return PersistentData.RaidLoadout; }

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Raid")
	int32 GetRaidGold() const { return PersistentData.RaidGold; }

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Raid")
	void ConfirmRaidLoadout(const TArray<FPDInventorySlot>& InLoadout, int32 InGold);

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Raid")
	void ClearRaidLoadout();

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Raid")
	bool IsExtracted() const { return bIsExtracted; }

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Raid")
	bool IsRaidDead() const { return bIsRaidDead; }

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Raid")
	void SetExtracted(bool bInExtracted);

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Raid")
	void SetRaidDead(bool bInDead);

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Raid")
	void ResetRaidParticipationState();

	// 결산/Hub UI가 동료의 추출·사망 전환을 실시간으로 받기 위한 알림. 서버/클라 모두 발화.
	UPROPERTY(BlueprintAssignable, Category="PD|PlayerState|Raid")
	FPDOnRaidParticipationChanged OnRaidParticipationChanged;

	// 결산 위젯 ACK (좌클릭) 상태. 서버 권한, 모든 클라에 리플리케이트.
	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Raid")
	bool IsTravelReady() const { return bIsTravelReady; }

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Raid")
	void SetTravelReady(bool bInReady);

	// 결산 위젯이 동료의 ACK 전환을 실시간으로 받기 위한 알림.
	UPROPERTY(BlueprintAssignable, Category="PD|PlayerState|Raid")
	FPDOnTravelReadyChanged OnTravelReadyChanged;

	// ─── Step 2-C: 라이드 결산 스탯 (서버 권한, 모든 클라에 리플리케이트) ─────
	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Raid")
	const FPDRaidStats& GetRaidStats() const { return RaidStats; }

	void SetRaidStats(const FPDRaidStats& InStats);
	void AddKill();
	void SetSurvivalSeconds(float Seconds);
	void SetGoldDelta(int32 Delta);
	void SetItemDelta(int32 Delta);

	// 서버 전용: StartRaid 시점에 GameMode 가 호출하여 시작 스냅샷 저장. 결산에서 Delta 계산용.
	void CaptureInitialRaidSnapshot(int32 InGold, int32 InItemQuantity);
	int32 GetInitialRaidGold() const { return RaidInitialGold; }
	int32 GetInitialRaidItemQuantity() const { return RaidInitialItemQuantity; }

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Stash")
	const TArray<FPDInventorySlot>& GetPersistentStashItems() const { return PersistentData.StashItems; }

	UFUNCTION(BlueprintPure, Category="PD|PlayerState|Stash")
	int32 GetPersistentStashUpgradeLevel() const { return FMath::Max(0, PersistentData.StashUpgradeLevel); }

	UFUNCTION(BlueprintCallable, Category="PD|PlayerState|Stash")
	void SetPersistentStashSnapshot(const TArray<FPDInventorySlot>& InStashItems, int32 InUpgradeLevel);

	int32 CountPersistentStashItems(FName ItemID, bool bCountAnyItem) const;
	int32 RemovePersistentStashItems(FName ItemID, int32 Quantity, bool bRemoveAnyItem);
	void TransferInventoryToPersistentStash(const UPDInventoryComponent* InventoryComponent);

	// 서버 전용: 현재 EquipmentComponent 장착 항목을 PersistentData.EquippedItems 에 캡처(탈출 시).
	void CaptureEquippedItemsToPersistent();

	// 서버 전용: PersistentData.EquippedItems 를 기존 장착 API 로 재장착(진입 시). 폰 소유 후 호출.
	void RestoreEquippedItemsFromPersistent();

	// 서버 전용: 퀵슬롯이 참조하는 인벤토리 아이템(보조무기/소모품)을 슬롯 위치와 함께 캡처(탈출 시).
	// 장착 무기를 참조하는 슬롯은 제외(장비 복원이 처리). BuildStashTransferItems 보다 먼저 호출.
	void CaptureQuickSlotItemsToPersistent();

	// 서버 전용: 인벤토리에서 스태시로 보낼 항목(= 퀵슬롯 보관 항목 제외)을 산출.
	void BuildStashTransferItems(TArray<FPDInventorySlot>& OutItems) const;

	// 서버 전용: PersistentData.QuickSlotKeptItems 를 인벤토리에 복원 + 퀵슬롯 재할당(진입 시). idempotent.
	void RestoreQuickSlotItemsFromPersistent();

protected:
	virtual void BeginPlay() override;

private:
	void ApplyComponentDataDefaults();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|PlayerState|Data", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UDataTable> ItemDataTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPDInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPDEquipmentComponent> EquipmentComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPDQuickSlotComponent> QuickSlotComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPDQuestComponent> QuestComponent;

	UPROPERTY(ReplicatedUsing=OnRep_PersistentData, VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState|Save", meta=(AllowPrivateAccess="true"))
	FPDPlayerData PersistentData;

	// 트래블 너머 안정적인 저장 키. 서버 최초 1회 할당, CopyProperties 로 보존.
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState|Save", meta=(AllowPrivateAccess="true"))
	FString PersistentSaveId;

	UPROPERTY(ReplicatedUsing=OnRep_RaidParticipation, VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState|Raid", meta=(AllowPrivateAccess="true"))
	bool bIsExtracted = false;

	UPROPERTY(ReplicatedUsing=OnRep_RaidParticipation, VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState|Raid", meta=(AllowPrivateAccess="true"))
	bool bIsRaidDead = false;

	// Step 2-C: 결산 위젯이 표시할 스탯. 전원이 서로 결과 보므로 COND_None 으로 모두에게 전파.
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState|Raid", meta=(AllowPrivateAccess="true"))
	FPDRaidStats RaidStats;

	// 결산 위젯의 ACK 상태. 좌클릭 시 true. 전원 true 면 GameMode 가 ServerTravel.
	UPROPERTY(ReplicatedUsing=OnRep_TravelReady, VisibleAnywhere, BlueprintReadOnly, Category="PD|PlayerState|Raid", meta=(AllowPrivateAccess="true"))
	bool bIsTravelReady = false;

	// 서버 전용 (UPROPERTY 아님 → 리플리케이트 안 됨). StartRaid 캡처 → 사망/추출 시 Delta 계산.
	int32 RaidInitialGold = 0;
	int32 RaidInitialItemQuantity = 0;

	UFUNCTION()
	void OnRep_PersistentData();

	UFUNCTION()
	void OnRep_RaidParticipation();

	UFUNCTION()
	void OnRep_TravelReady();
};
