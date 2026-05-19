#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Type/Types.h"
#include "PDInventoryDragDropOperation.generated.h"

UENUM(BlueprintType)
enum class EPDItemContainerType : uint8
{
	None,
	Inventory,
	Stash,
	QuickSlot,
	Equipment,
	SecureContainer,
	Loot
};

UCLASS(BlueprintType)
class PROJECTD_API UPDInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "PD|Inventory|Drag")
	EPDItemContainerType SourceContainerType = EPDItemContainerType::None;

	UPROPERTY(BlueprintReadWrite, Category = "PD|Inventory|Drag")
	int32 SourceSlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, Category = "PD|Inventory|Drag")
	FPDInventorySlot SlotData;

	UFUNCTION(BlueprintPure, Category = "PD|Inventory|Drag")
	bool IsValidPayload() const
	{
		return SourceContainerType != EPDItemContainerType::None && SourceSlotIndex != INDEX_NONE && !SlotData.IsEmpty();
	}
};
