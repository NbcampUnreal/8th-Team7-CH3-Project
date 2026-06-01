#include "Core/PDGameInstance.h"
#include "AbilitySystemGlobals.h"
#include "Core/PDPlayerState.h"
#include "Core/PDGameUserSettings.h"
#include "Core/PDLocalSessionService.h"
#include "Core/PDSaveGame.h"
#include "Core/PDSessionService.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Data/PDItemSlotTransfer.h"
#include "Misc/PackageName.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/PDLoadingScreenSubsystem.h"

void UPDGameInstance::Init()
{
	Super::Init();
	UAbilitySystemGlobals::Get().InitGlobalData();
	LoadFromDisk();

	const TSubclassOf<UPDSessionService> ResolvedClass =
		SessionServiceClass ? SessionServiceClass : TSubclassOf<UPDSessionService>(UPDLocalSessionService::StaticClass());
	ActiveSessionService = NewObject<UPDSessionService>(this, ResolvedClass);

	// 저장된 전체 볼륨을 게임 시작 시 오디오 디바이스에 반영(transient라 매 실행마다 재적용 필요).
	if (UPDGameUserSettings* UserSettings = UPDGameUserSettings::Get())
	{
		UserSettings->ApplyAudioSettings();
	}
}

void UPDGameInstance::SetPlayerData(const FPDPlayerData& InData)
{
	PlayerData=InData;
	FPDItemContainerOps::EnsureInstanceIDs(PlayerData.StashItems);
	FPDItemContainerOps::EnsureInstanceIDs(PlayerData.RaidLoadout);
}

FString UPDGameInstance::GetSaveKeyForController(const APlayerController* PlayerController) const
{
	if (!PlayerController)
	{
		return TEXT("LocalPlayer");
	}

	// 트래블 너머 안정 키 1순위: UniqueNetId (로그인 신원 — non-seamless 재접속에도 동일하게 재생성).
	// PIE listen-server 는 seamless travel 이 동작하지 않아 CopyProperties 가 안 불리므로 UniqueNetId 를 우선.
	if (const APlayerState* PlayerState = PlayerController->PlayerState)
	{
		const FUniqueNetIdRepl& UniqueId = PlayerState->GetUniqueId();
		if (UniqueId.IsValid())
		{
			return UniqueId.ToString();
		}
	}

	// 2순위: PersistentSaveId (seamless travel 환경에서 CopyProperties 로 보존되는 경우).
	if (const APDPlayerState* PDPlayerState = Cast<APDPlayerState>(PlayerController->PlayerState))
	{
		const FString& SaveId = PDPlayerState->GetPersistentSaveId();
		if (!SaveId.IsEmpty())
		{
			return SaveId;
		}
	}

	// 3순위(불안정 폴백): PlayerName_PlayerId.
	if (const APlayerState* PlayerState = PlayerController->PlayerState)
	{
		const FString PlayerName = PlayerState->GetPlayerName();
		if (!PlayerName.IsEmpty())
		{
			return FString::Printf(TEXT("%s_%d"), *PlayerName, PlayerState->GetPlayerId());
		}
	}

	return PlayerController->GetName();
}

FPDPlayerData UPDGameInstance::LoadPlayerDataFromDisk(const FString& SaveKey, bool bUseLegacyFallback) const
{
	const FString SlotName = MakePlayerSlotName(SaveKey);
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, UPDSaveGame::UserIndex))
	{
		return bUseLegacyFallback ? PlayerData : FPDPlayerData();
	}

	const UPDSaveGame* SaveObject = Cast<UPDSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UPDSaveGame::UserIndex));
	FPDPlayerData LoadedData = SaveObject ? SaveObject->PlayerData : (bUseLegacyFallback ? PlayerData : FPDPlayerData());
	FPDItemContainerOps::EnsureInstanceIDs(LoadedData.StashItems);
	FPDItemContainerOps::EnsureInstanceIDs(LoadedData.RaidLoadout);
	return LoadedData;
}

void UPDGameInstance::SavePlayerDataToDisk(const FString& SaveKey, const FPDPlayerData& InData) const
{
	UPDSaveGame* SaveObject = Cast<UPDSaveGame>(UGameplayStatics::CreateSaveGameObject(UPDSaveGame::StaticClass()));
	if (!SaveObject)
	{
		return;
	}

	SaveObject->PlayerData = InData;
	UGameplayStatics::SaveGameToSlot(SaveObject, MakePlayerSlotName(SaveKey), UPDSaveGame::UserIndex);
}

void UPDGameInstance::SetStashItems(const TArray<FPDInventorySlot>& InStashItems)
{
	PlayerData.StashItems=InStashItems;
	FPDItemContainerOps::EnsureInstanceIDs(PlayerData.StashItems);
}

const TArray<FPDInventorySlot>& UPDGameInstance::GetStashItems() const
{
	return PlayerData.StashItems;
}

// 컴팩트 스태시 배열에 소스 슬롯들을 병합(스택 채우기 → 새 슬롯 append). 빈 슬롯은 무시.
// 호출 전 Stash 는 컴팩트(빈 슬롯 제거) 상태여야 함.
static void MergeSlotsIntoStashArray(TArray<FPDInventorySlot>& Stash, const TArray<FPDInventorySlot>& SourceSlots)
{
	for (const FPDInventorySlot& RaidSlot : SourceSlots)
	{
		if (RaidSlot.IsEmpty())
		{
			continue;
		}

		int32 RemainingQuantity = RaidSlot.Quantity;

		if (RaidSlot.ItemData.MaxStack > 1)
		{
			for (FPDInventorySlot& StashSlot : Stash)
			{
				if (StashSlot.IsEmpty() || StashSlot.ItemData.ItemID != RaidSlot.ItemData.ItemID)
				{
					continue;
				}

				const int32 StackSpace = StashSlot.ItemData.MaxStack - StashSlot.Quantity;
				if (StackSpace <= 0)
				{
					continue;
				}

				const int32 AddedQuantity = FMath::Min(StackSpace, RemainingQuantity);
				StashSlot.Quantity += AddedQuantity;
				StashSlot.bIsEmpty = false;
				RemainingQuantity -= AddedQuantity;

				if (RemainingQuantity <= 0)
				{
					break;
				}
			}
		}

		while (RemainingQuantity > 0)
		{
			FPDInventorySlot NewSlot;
			NewSlot.ItemInstanceID = RaidSlot.ItemInstanceID;
			NewSlot.ItemData = RaidSlot.ItemData;
			NewSlot.ModificationLevel = RaidSlot.ModificationLevel;
			NewSlot.Quantity = FMath::Min(RemainingQuantity, FMath::Max(1, RaidSlot.ItemData.MaxStack));
			NewSlot.bIsEmpty = false;
			NewSlot.EnsureInstanceID();

			Stash.Add(NewSlot);
			RemainingQuantity -= NewSlot.Quantity;
		}
	}
}

void UPDGameInstance::MergeSlotsIntoStash(const TArray<FPDInventorySlot>& Slots, int32 GoldToAdd)
{
	// 스태시 컴포넌트가 그리드 크기로 패딩한 빈 슬롯을 먼저 제거 → 컴팩트 상태에서 병합.
	// (append 후 InitializeStash 의 SetNum 이 그리드 초과분을 잘라내는 문제 방지.)
	TArray<FPDInventorySlot>& Stash = PlayerData.StashItems;
	Stash.RemoveAll([](const FPDInventorySlot& Slot) { return Slot.IsEmpty(); });

	MergeSlotsIntoStashArray(Stash, Slots);

	PlayerData.Gold += GoldToAdd;
	FPDItemContainerOps::EnsureInstanceIDs(PlayerData.StashItems);
}

void UPDGameInstance::SetStashUpgradeLevel(int32 InUpgradeLevel)
{
	PlayerData.StashUpgradeLevel = FMath::Max(0, InUpgradeLevel);
}

int32 UPDGameInstance::GetStashUpgradeLevel() const
{
	return FMath::Max(0, PlayerData.StashUpgradeLevel);
}

void UPDGameInstance::SetSecureContainerItems(const TArray<FPDInventorySlot>& InSecureContainerItems)
{
	SecureContainerItems = InSecureContainerItems;
	FPDItemContainerOps::EnsureInstanceIDs(SecureContainerItems);
}

const TArray<FPDInventorySlot>& UPDGameInstance::GetSecureContainerItems() const
{
	return SecureContainerItems;
}

void UPDGameInstance::SetTraderReputation(int32 InLevel, int32 InExp)
{
	PlayerData.TraderReputationLevel = FMath::Max(1, InLevel);
	PlayerData.TraderReputationExp = FMath::Max(0, InExp);
}

void UPDGameInstance::SetTraderReputationExp(int32 InExp)
{
	PlayerData.TraderReputationExp = FMath::Max(0, InExp);
}

void UPDGameInstance::SetTraderReputationLevel(int32 InLevel)
{
	PlayerData.TraderReputationLevel = FMath::Max(1, InLevel);
}

int32 UPDGameInstance::GetTraderReputationExp() const
{
	return FMath::Max(0, PlayerData.TraderReputationExp);
}

int32 UPDGameInstance::GetTraderReputationLevel() const
{
	return FMath::Max(1, PlayerData.TraderReputationLevel);
}

void UPDGameInstance::ConfirmRaidLoadout(const TArray<FPDInventorySlot>& InLoadout, int32 InGold)
{
	PlayerData.RaidLoadout = InLoadout;
	FPDItemContainerOps::EnsureInstanceIDs(PlayerData.RaidLoadout);
	PlayerData.RaidGold    = FMath::Max(0, InGold);

	PlayerData.Gold = FMath::Max(0, PlayerData.Gold - PlayerData.RaidGold);

	for (const FPDInventorySlot& LoadoutSlot : PlayerData.RaidLoadout)
	{
		if (LoadoutSlot.IsEmpty()) continue;
		int32 ToRemove = LoadoutSlot.Quantity;
		if (LoadoutSlot.ItemInstanceID.IsValid())
		{
			for (FPDInventorySlot& StashSlot : PlayerData.StashItems)
			{
				if (StashSlot.IsEmpty() || StashSlot.ItemInstanceID != LoadoutSlot.ItemInstanceID) continue;
				const int32 Removed = FMath::Min(StashSlot.Quantity, ToRemove);
				StashSlot.Quantity -= Removed;
				if (StashSlot.Quantity <= 0) StashSlot.Clear();
				ToRemove -= Removed;
				break;
			}
		}

		for (FPDInventorySlot& StashSlot : PlayerData.StashItems)
		{
			if (ToRemove <= 0) break;
			if (StashSlot.IsEmpty()) continue;
			if (StashSlot.ItemData.ItemID != LoadoutSlot.ItemData.ItemID) continue;
			int32 Removed = FMath::Min(StashSlot.Quantity, ToRemove);
			StashSlot.Quantity -= Removed;
			if (StashSlot.Quantity <= 0) StashSlot.Clear();
			ToRemove -= Removed;
			if (ToRemove <= 0) break;
		}
	}

	SaveToDisk();
}

void UPDGameInstance::ClearRaidLoadout()
{
	PlayerData.RaidLoadout.Empty();
	PlayerData.RaidGold = 0;
}

void UPDGameInstance::SaveToDisk()
{
	UPDSaveGame* SaveObject=Cast<UPDSaveGame>(UGameplayStatics::CreateSaveGameObject(UPDSaveGame::StaticClass()));
	if (!SaveObject) return;
	SaveObject->PlayerData=PlayerData;
	UGameplayStatics::SaveGameToSlot(SaveObject, UPDSaveGame::SlotName, UPDSaveGame::UserIndex);
}

void UPDGameInstance::LoadFromDisk()
{
	if (!UGameplayStatics::DoesSaveGameExist(UPDSaveGame::SlotName, UPDSaveGame::UserIndex)) return;
	UPDSaveGame* SaveObject=Cast<UPDSaveGame>(UGameplayStatics::LoadGameFromSlot(UPDSaveGame::SlotName, UPDSaveGame::UserIndex));
	if (!SaveObject) return;
	PlayerData=SaveObject->PlayerData;
	FPDItemContainerOps::EnsureInstanceIDs(PlayerData.StashItems);
	FPDItemContainerOps::EnsureInstanceIDs(PlayerData.RaidLoadout);
	// SecureContainer???�이브파?�이 ?�닌 ?�션 ?��??�만 존재?�디?�크 로드?�는 리셋?�서 ?�이?�이 ?�아?��? ?�도�???
	SecureContainerItems.Reset();
}

void UPDGameInstance::TravelToLevel(TSoftObjectPtr<UWorld> Level, bool bMarkBaseResetPending)
{
	if (Level.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDGameInstance::TravelToLevel: Level is not set."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (bMarkBaseResetPending) bPendingResetToBase=true;

	if (World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDGameInstance::TravelToLevel: clients must request travel through the server."));
		return;
	}

	// 호스트 측 LoadingScreen을 viewport에 즉시 추가.
	// (클라이언트 측은 APDPlayerController::PreClientTravel에서 자동 처리)
	if (UPDLoadingScreenSubsystem* LSS = GetSubsystem<UPDLoadingScreenSubsystem>())
	{
		LSS->ShowImmediate();
	}

	// 한 프레임 지연 후 실제 트래블 — LoadingScreen이 먼저 렌더된 뒤 ServerTravel/OpenLevel cleanup이 시작되도록.
	// 즉시 ServerTravel을 호출하면 같은 프레임에 게임 스레드가 cleanup으로 점유되어 LoadingScreen이 한 프레임도 안 보임.
	World->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this, Level]()
		{
			UWorld* W = GetWorld();
			if (!W) return;

			if (W->GetNetMode() != NM_Standalone)
			{
				const FString LevelPackageName = FPackageName::ObjectPathToPackageName(Level.ToSoftObjectPath().ToString());
				W->ServerTravel(LevelPackageName);
			}
			else
			{
				UGameplayStatics::OpenLevelBySoftObjectPtr(this, Level);
			}
		}));
}

bool UPDGameInstance::ConsumePendingResetToBase()
{
	const bool bWasPending=bPendingResetToBase;
	bPendingResetToBase=false;
	return bWasPending;
}

bool UPDGameInstance::ConsumePendingRoomScreen()
{
	const bool bWas = bPendingRoomScreen;
	bPendingRoomScreen = false;
	return bWas;
}

FString UPDGameInstance::MakePlayerSlotName(const FString& SaveKey) const
{
	return FString::Printf(TEXT("%s_%s"), *UPDSaveGame::SlotName, *SanitizeSaveKey(SaveKey));
}

FString UPDGameInstance::SanitizeSaveKey(const FString& SaveKey) const
{
	FString Result = SaveKey.IsEmpty() ? TEXT("LocalPlayer") : SaveKey;
	const TCHAR InvalidChars[] = TEXT("\\/:*?\"<>|. ");
	for (int32 Index = 0; InvalidChars[Index] != TCHAR('\0'); ++Index)
	{
		Result.ReplaceCharInline(InvalidChars[Index], TCHAR('_'));
	}
	return Result;
}
