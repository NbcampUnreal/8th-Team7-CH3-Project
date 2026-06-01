#include "Core/PDPlayerComponentResolver.h"

#include "Characters/PDPlayerCharacter.h"
#include "Components/ActorComponent.h"
#include "Core/PDPlayerController.h"
#include "Core/PDPlayerState.h"
#include "Data/PDQuestComponent.h"
#include "GameFramework/Pawn.h"
#include "Items/Equipment/PDEquipmentComponent.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Containers/PDQuickSlotComponent.h"

namespace
{
	AActor* ResolveActor(const UObject* Context)
	{
		if (!Context)
		{
			return nullptr;
		}

		if (AActor* Actor = const_cast<AActor*>(Cast<AActor>(Context)))
		{
			return Actor;
		}

		if (const UActorComponent* Component = Cast<UActorComponent>(Context))
		{
			return Component->GetOwner();
		}

		return nullptr;
	}
}

APDPlayerState* FPDPlayerComponentResolver::ResolvePlayerState(const UObject* Context)
{
	AActor* Actor = ResolveActor(Context);
	if (!Actor)
	{
		return nullptr;
	}

	if (APDPlayerState* PDPlayerState = Cast<APDPlayerState>(Actor))
	{
		return PDPlayerState;
	}

	if (const APDPlayerController* PDController = Cast<APDPlayerController>(Actor))
	{
		return PDController->GetPDPlayerState();
	}

	if (const APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(Actor))
	{
		return PlayerCharacter->GetPlayerState<APDPlayerState>();
	}

	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		return Pawn->GetPlayerState<APDPlayerState>();
	}

	return nullptr;
}

APDPlayerCharacter* FPDPlayerComponentResolver::ResolvePlayerCharacter(const UObject* Context)
{
	AActor* Actor = ResolveActor(Context);
	if (!Actor)
	{
		return nullptr;
	}

	if (APDPlayerCharacter* PlayerCharacter = Cast<APDPlayerCharacter>(Actor))
	{
		return PlayerCharacter;
	}

	if (const APDPlayerState* PDPlayerState = Cast<APDPlayerState>(Actor))
	{
		return PDPlayerState->GetPDPlayerCharacter();
	}

	if (const APDPlayerController* PDController = Cast<APDPlayerController>(Actor))
	{
		return Cast<APDPlayerCharacter>(PDController->GetPawn());
	}

	return nullptr;
}

UPDInventoryComponent* FPDPlayerComponentResolver::ResolveInventory(const UObject* Context)
{
	if (APDPlayerState* PDPlayerState = ResolvePlayerState(Context))
	{
		if (UPDInventoryComponent* InventoryComponent = PDPlayerState->GetInventoryComponent())
		{
			return InventoryComponent;
		}
	}

	if (const APDPlayerController* PDController = Cast<APDPlayerController>(ResolveActor(Context)))
	{
		if (UPDInventoryComponent* InventoryComponent = PDController->GetPlayerInventoryComponent())
		{
			return InventoryComponent;
		}
	}

	if (APDPlayerCharacter* PlayerCharacter = ResolvePlayerCharacter(Context))
	{
		if (UPDInventoryComponent* InventoryComponent = PlayerCharacter->GetInventoryComponent())
		{
			return InventoryComponent;
		}
	}

	AActor* Actor = ResolveActor(Context);
	return Actor ? Actor->FindComponentByClass<UPDInventoryComponent>() : nullptr;
}

UPDEquipmentComponent* FPDPlayerComponentResolver::ResolveEquipment(const UObject* Context)
{
	if (APDPlayerState* PDPlayerState = ResolvePlayerState(Context))
	{
		if (UPDEquipmentComponent* EquipmentComponent = PDPlayerState->GetEquipmentComponent())
		{
			return EquipmentComponent;
		}
	}

	if (const APDPlayerController* PDController = Cast<APDPlayerController>(ResolveActor(Context)))
	{
		if (UPDEquipmentComponent* EquipmentComponent = PDController->GetPlayerEquipmentComponent())
		{
			return EquipmentComponent;
		}
	}

	if (APDPlayerCharacter* PlayerCharacter = ResolvePlayerCharacter(Context))
	{
		if (UPDEquipmentComponent* EquipmentComponent = PlayerCharacter->GetEquipmentComponent())
		{
			return EquipmentComponent;
		}
	}

	AActor* Actor = ResolveActor(Context);
	return Actor ? Actor->FindComponentByClass<UPDEquipmentComponent>() : nullptr;
}

UPDQuickSlotComponent* FPDPlayerComponentResolver::ResolveQuickSlot(const UObject* Context)
{
	if (APDPlayerState* PDPlayerState = ResolvePlayerState(Context))
	{
		if (UPDQuickSlotComponent* QuickSlotComponent = PDPlayerState->GetQuickSlotComponent())
		{
			return QuickSlotComponent;
		}
	}

	if (const APDPlayerController* PDController = Cast<APDPlayerController>(ResolveActor(Context)))
	{
		if (UPDQuickSlotComponent* QuickSlotComponent = PDController->GetPlayerQuickSlotComponent())
		{
			return QuickSlotComponent;
		}
	}

	if (APDPlayerCharacter* PlayerCharacter = ResolvePlayerCharacter(Context))
	{
		if (UPDQuickSlotComponent* QuickSlotComponent = PlayerCharacter->GetQuickSlotComponent())
		{
			return QuickSlotComponent;
		}
	}

	AActor* Actor = ResolveActor(Context);
	return Actor ? Actor->FindComponentByClass<UPDQuickSlotComponent>() : nullptr;
}

UPDQuestComponent* FPDPlayerComponentResolver::ResolveQuest(const UObject* Context)
{
	if (APDPlayerState* PDPlayerState = ResolvePlayerState(Context))
	{
		if (UPDQuestComponent* QuestComponent = PDPlayerState->GetQuestComponent())
		{
			return QuestComponent;
		}
	}

	if (const APDPlayerController* PDController = Cast<APDPlayerController>(ResolveActor(Context)))
	{
		if (UPDQuestComponent* QuestComponent = PDController->GetPlayerQuestComponent())
		{
			return QuestComponent;
		}
	}

	if (APDPlayerCharacter* PlayerCharacter = ResolvePlayerCharacter(Context))
	{
		if (UPDQuestComponent* QuestComponent = PlayerCharacter->GetQuestComponent())
		{
			return QuestComponent;
		}
	}

	AActor* Actor = ResolveActor(Context);
	return Actor ? Actor->FindComponentByClass<UPDQuestComponent>() : nullptr;
}
