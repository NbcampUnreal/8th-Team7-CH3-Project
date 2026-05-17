#include "Core/PDGameMode.h"
#include "Core/PDGameState.h"
#include "Core/PDGameInstance.h"
#include "Items/PDInventoryComponent.h"
#include "GameFramework/Pawn.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Type/Types.h"
#include "Widgets/PDActivatableBase.h"

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
	SetRaidState(ERaidState::Ended);

	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();

	if (bSuccess)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			TransferPlayerInventoryToStash(PC);
	}
	if (GI) GI->SaveToDisk();

	// BP 연출 훅 — BP에서 결과 UI + OpenLevel 처리
	OnRaidEnded(bSuccess);
}

void APDGameMode::OnPlayerDied(APlayerController* PC, AActor* Killer)
{
	if (!PC) return;
	EndRaid(false);

	if (DeathScreenClass)
	{
		if (UPDFrontendUISubsystem* UI = UPDFrontendUISubsystem::Get(this))
		{
			UI->PushToLayer(EUILayer::Frontend, DeathScreenClass);
		}
	}
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
		if (RaidSlot.IsEmpty()) continue;

		int32 Remaining = RaidSlot.Quantity;
		
		if (RaidSlot.ItemData.MaxStack > 1)
		{
			for (FPDInventorySlot& StashSlot : StashItems)
			{
				if (StashSlot.IsEmpty()) continue;
				if (StashSlot.ItemData.ItemID != RaidSlot.ItemData.ItemID) continue;
				int32 Space = StashSlot.ItemData.MaxStack - StashSlot.Quantity;
				if (Space <= 0) continue;
				int32 ToAdd = FMath::Min(Space, Remaining);
				StashSlot.Quantity += ToAdd;
				StashSlot.bIsEmpty  = false;
				Remaining          -= ToAdd;
				if (Remaining <= 0) break;
			}
		}
		
		while (Remaining > 0)
		{
			FPDInventorySlot NewSlot;
			NewSlot.ItemData          = RaidSlot.ItemData;
			NewSlot.ModificationLevel = RaidSlot.ModificationLevel;
			NewSlot.Quantity          = FMath::Min(Remaining, FMath::Max(1, RaidSlot.ItemData.MaxStack));
			NewSlot.bIsEmpty          = false;
			StashItems.Add(NewSlot);
			Remaining -= NewSlot.Quantity;
		}
	}

	GI->SetStashItems(StashItems);

	FPDPlayerData Data = GI->GetPlayerData();
	Data.Gold += Inventory->Gold;
	GI->SetPlayerData(Data);
}
