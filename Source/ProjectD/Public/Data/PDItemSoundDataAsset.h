#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Type/Types.h"
#include "PDItemSoundDataAsset.generated.h"

class USoundBase;

USTRUCT(BlueprintType)
struct FPDItemMoveSoundSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Item Sound")
	TObjectPtr<USoundBase> MoveSound = nullptr;
};

USTRUCT(BlueprintType)
struct FPDConsumableItemSoundSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Item Sound")
	TObjectPtr<USoundBase> MoveSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Item Sound")
	TObjectPtr<USoundBase> UseSound = nullptr;
};

UCLASS(BlueprintType)
class PROJECTD_API UPDItemSoundDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="PD|Item Sound")
	USoundBase* GetMoveSound(EPDItemType ItemType) const;

	UFUNCTION(BlueprintPure, Category="PD|Item Sound")
	USoundBase* GetConsumableUseSound() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Item Sound")
	FPDItemMoveSoundSet Equipment;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Item Sound")
	FPDConsumableItemSoundSet Consumable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Item Sound")
	FPDItemMoveSoundSet Misc;
};
