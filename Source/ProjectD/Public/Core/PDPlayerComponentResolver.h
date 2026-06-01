#pragma once

#include "CoreMinimal.h"

class APDPlayerCharacter;
class APDPlayerState;
class UPDEquipmentComponent;
class UPDInventoryComponent;
class UPDQuestComponent;
class UPDQuickSlotComponent;

struct PROJECTD_API FPDPlayerComponentResolver
{
	static APDPlayerState* ResolvePlayerState(const UObject* Context);
	static APDPlayerCharacter* ResolvePlayerCharacter(const UObject* Context);
	static UPDInventoryComponent* ResolveInventory(const UObject* Context);
	static UPDEquipmentComponent* ResolveEquipment(const UObject* Context);
	static UPDQuickSlotComponent* ResolveQuickSlot(const UObject* Context);
	static UPDQuestComponent* ResolveQuest(const UObject* Context);
};
