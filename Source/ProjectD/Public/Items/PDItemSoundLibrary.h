#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Type/Types.h"
#include "PDItemSoundLibrary.generated.h"

class USoundBase;
class UAudioComponent;

UCLASS()
class PROJECTD_API UPDItemSoundLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="PD|Item Sound", meta=(WorldContext="WorldContextObject"))
	static void PlayItemMoveSound(const UObject* WorldContextObject, const FPDItemData& ItemData);

	UFUNCTION(BlueprintCallable, Category="PD|Item Sound", meta=(WorldContext="WorldContextObject"))
	static void PlayConsumableUseSound(const UObject* WorldContextObject, const FPDItemData& ItemData);

	UFUNCTION(BlueprintCallable, Category="PD|Item Sound", meta=(WorldContext="WorldContextObject"))
	static UAudioComponent* SpawnConsumableUseSound(const UObject* WorldContextObject, const FPDItemData& ItemData);

	UFUNCTION(BlueprintPure, Category="PD|Item Sound")
	static USoundBase* ResolveItemMoveSound(EPDItemType ItemType);

	UFUNCTION(BlueprintPure, Category="PD|Item Sound")
	static USoundBase* ResolveConsumableUseSound();
};
