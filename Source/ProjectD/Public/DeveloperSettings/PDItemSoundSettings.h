#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PDItemSoundSettings.generated.h"

class UPDItemSoundDataAsset;

UCLASS(config=Game, defaultconfig, meta=(DisplayName="PD Item Sound Settings"))
class PROJECTD_API UPDItemSoundSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Item Sound")
	TSoftObjectPtr<UPDItemSoundDataAsset> ItemSoundDataAsset;
};
