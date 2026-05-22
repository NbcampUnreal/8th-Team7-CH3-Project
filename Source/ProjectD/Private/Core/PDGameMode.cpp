#include "Core/PDGameMode.h"
#include "Core/PDGameState.h"
#include "Core/PDGameInstance.h"
#include "Items/PDInventoryComponent.h"
#include "Items/PDSecureContainerComponent.h"
#include "GameFramework/Pawn.h"
#include "Type/Types.h"

namespace
{
	void AddSlotToStashItems(TArray<FPDInventorySlot>& StashItems, const FPDInventorySlot& SourceSlot)
	{
		if (SourceSlot.IsEmpty())
		{
			return;
		}

		int32 Remaining = SourceSlot.Quantity;

		if (SourceSlot.ItemData.MaxStack > 1)
		{
			for (FPDInventorySlot& StashSlot : StashItems)
			{
				if (StashSlot.IsEmpty() || StashSlot.ItemData.ItemID != SourceSlot.ItemData.ItemID)
				{
					continue;
				}

				const int32 Space = StashSlot.ItemData.MaxStack - StashSlot.Quantity;
				if (Space <= 0)
				{
					continue;
				}

				const int32 ToAdd = FMath::Min(Space, Remaining);
				StashSlot.Quantity += ToAdd;
				StashSlot.bIsEmpty = false;
				Remaining -= ToAdd;

				if (Remaining <= 0)
				{
					break;
				}
			}
		}

		while (Remaining > 0)
		{
			FPDInventorySlot NewSlot;
			NewSlot.ItemData = SourceSlot.ItemData;
			NewSlot.ModificationLevel = SourceSlot.ModificationLevel;
			NewSlot.Quantity = FMath::Min(Remaining, FMath::Max(1, SourceSlot.ItemData.MaxStack));
			NewSlot.bIsEmpty = false;
			StashItems.Add(NewSlot);
			Remaining -= NewSlot.Quantity;
		}
	}
}

APDGameMode::APDGameMode()
{
}

void APDGameMode::StartRaid()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		InitializePlayerInventoryFromLoadout(PC);

	SetRaidState(ERaidState::InProgress);
}

void APDGameMode::RequestExtraction(APlayerController* PC)
{
	if (!PC||CurrentRaidState!=ERaidState::InProgress) return;
	SetRaidState(ERaidState::Extracting);
	EndRaid(true);
}

void APDGameMode::EndRaid(bool bSuccess)
{
	if (CurrentRaidState == ERaidState::Ended) return;
	SetRaidState(ERaidState::Ended);

	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (bSuccess)
		{
			TransferPlayerInventoryToStash(PC);
		}

		if (APawn* Pawn = PC->GetPawn())
		{
			if (UPDSecureContainerComponent* SecureContainer = Pawn->FindComponentByClass<UPDSecureContainerComponent>())
			{
				SecureContainer->SaveSecureContainer();
			}
		}
	}

	if (GI) GI->SaveToDisk();

	// BP 연출 훅 — BP에서 결과 UI 표시. 트래블은 ACK 게이트로 위임.
	OnRaidEnded(bSuccess);

	// 모든 PC가 ACK하거나 타임아웃 시 ServerTravel. 둘 중 빠른 쪽이 발동.
	GetWorldTimerManager().SetTimer(
		TravelTimeoutTimerHandle,
		this,
		&APDGameMode::OnTravelTimeoutExpired,
		TravelGateTimeoutSeconds,
		false);
}

void APDGameMode::OnPlayerDied(APlayerController* PC, AActor* Killer)
{
	if (!PC) return;
	if (CurrentRaidState == ERaidState::Ended) return;
	EndRaid(false);
}

void APDGameMode::SetRaidState(ERaidState NewState)
{
	if (CurrentRaidState==NewState) return;
	CurrentRaidState=NewState;
	if (APDGameState* GS=GetGameState<APDGameState>()) GS->SetRaidState(NewState);
	OnRaidStateChanged(NewState);
}


void APDGameMode::InitializePlayerInventoryFromLoadout(APlayerController* PC)
{
	if (!PC) return;
	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	UPDInventoryComponent* Inventory = Pawn->FindComponentByClass<UPDInventoryComponent>();
	if (!Inventory) return;

	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!GI) return;
	
	Inventory->ResetInventory();

	for (const FPDInventorySlot& Slot : GI->GetRaidLoadout())
	{
		if (!Slot.IsEmpty())
			Inventory->AddItem(Slot.ItemData, Slot.Quantity);
	}
	
	Inventory->Gold = GI->GetRaidGold();
	GI->ClearRaidLoadout();
}

void APDGameMode::TransferPlayerInventoryToStash(APlayerController* PC)
{
	if (!PC) return;
	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	UPDInventoryComponent* Inventory = Pawn->FindComponentByClass<UPDInventoryComponent>();
	if (!Inventory) return;

	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!GI) return;

	TArray<FPDInventorySlot> StashItems = GI->GetStashItems();

	for (const FPDInventorySlot& RaidSlot : Inventory->Items)
	{
		AddSlotToStashItems(StashItems, RaidSlot);
	}

	GI->SetStashItems(StashItems);

	FPDPlayerData Data = GI->GetPlayerData();
	Data.Gold += Inventory->Gold;
	GI->SetPlayerData(Data);
}

void APDGameMode::NotifyPlayerReadyForTravel(APlayerController* PC)
{
	if (!PC || bTravelStarted) return;

	ReadyPlayersForTravel.AddUnique(PC);

	if (AreAllPlayersReadyForTravel())
	{
		RequestBaseTravel();
	}
}

bool APDGameMode::AreAllPlayersReadyForTravel() const
{
	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	if (!GS) return false;

	const int32 TotalPlayers = GS->PlayerArray.Num();
	if (TotalPlayers <= 0) return false;

	int32 ValidReadyCount = 0;
	for (const TWeakObjectPtr<APlayerController>& WeakPC : ReadyPlayersForTravel)
	{
		if (WeakPC.IsValid()) ++ValidReadyCount;
	}
	return ValidReadyCount >= TotalPlayers;
}

void APDGameMode::OnTravelTimeoutExpired()
{
	if (bTravelStarted) return;
	UE_LOG(LogTemp, Warning, TEXT("APDGameMode: Travel gate timeout — forcing ServerTravel"));
	RequestBaseTravel();
}

void APDGameMode::RequestBaseTravel()
{
	if (bTravelStarted) return;
	bTravelStarted = true;

	GetWorldTimerManager().ClearTimer(TravelTimeoutTimerHandle);

	if (UPDGameInstance* GI = GetGameInstance<UPDGameInstance>())
	{
		GI->TravelToLevel(GI->GetBaseLevel(), /*bMarkBaseResetPending=*/false);
	}
}
