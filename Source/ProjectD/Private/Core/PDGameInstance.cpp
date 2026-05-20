#include "Core/PDGameInstance.h"
#include "Core/PDSaveGame.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UPDGameInstance::Init()
{
	Super::Init();
	LoadFromDisk();
}

void UPDGameInstance::SetPlayerData(const FPDPlayerData& InData)
{
	PlayerData=InData;
}

void UPDGameInstance::SetStashItems(const TArray<FPDInventorySlot>& InStashItems)
{
	PlayerData.StashItems=InStashItems;
}

const TArray<FPDInventorySlot>& UPDGameInstance::GetStashItems() const
{
	return PlayerData.StashItems;
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
	PlayerData.RaidGold    = FMath::Max(0, InGold);
	
	PlayerData.Gold = FMath::Max(0, PlayerData.Gold - PlayerData.RaidGold);
	
	for (const FPDInventorySlot& LoadoutSlot : InLoadout)
	{
		if (LoadoutSlot.IsEmpty()) continue;
		int32 ToRemove = LoadoutSlot.Quantity;
		for (FPDInventorySlot& StashSlot : PlayerData.StashItems)
		{
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
	SecureContainerItems.Reset();
}

void UPDGameInstance::TravelToLevel(TSoftObjectPtr<UWorld> Level, bool bMarkBaseResetPending)
{
	if (Level.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDGameInstance::TravelToLevel: Level is not set."));
		return;
	}
	if (bMarkBaseResetPending) bPendingResetToBase=true;
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Level);
}

bool UPDGameInstance::HostHamachiGame(TSoftObjectPtr<UWorld> Level, int32 Port)
{
	if (Level.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDGameInstance::HostHamachiGame: Level is not set."));
		return false;
	}

	const int32 ClampedPort = FMath::Clamp(Port, 1, 65535);
	const FString Options = FString::Printf(TEXT("listen?Port=%d"), ClampedPort);
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Level, true, Options);
	return true;
}

bool UPDGameInstance::JoinHamachiGame(const FString& HostAddress, int32 Port)
{
	FString Address = HostAddress;
	Address.TrimStartAndEndInline();

	if (Address.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("UPDGameInstance::JoinHamachiGame: HostAddress is empty."));
		return false;
	}

	if (APlayerController* PC = GetFirstLocalPlayerController())
	{
		if (!Address.Contains(TEXT(":")))
		{
			const int32 ClampedPort = FMath::Clamp(Port, 1, 65535);
			Address = FString::Printf(TEXT("%s:%d"), *Address, ClampedPort);
		}

		PC->ClientTravel(Address, TRAVEL_Absolute);
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("UPDGameInstance::JoinHamachiGame: local PlayerController is not valid."));
	return false;
}

bool UPDGameInstance::ConsumePendingResetToBase()
{
	const bool bWasPending=bPendingResetToBase;
	bPendingResetToBase=false;
	return bWasPending;
}
