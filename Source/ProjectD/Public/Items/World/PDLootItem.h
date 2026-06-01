#pragma once

#include "CoreMinimal.h"
#include "Items/World/PDItemBase.h"
#include "PDLootItem.generated.h"

UCLASS(Blueprintable)
class PROJECTD_API APDLootItem : public APDItemBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Item")
	void SetItemID(FName NewItemID);
};
