#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PDUISoundSettings.generated.h"

class USoundBase;

UCLASS(config=Game, defaultconfig, meta=(DisplayName="PD UI Sound Settings"))
class PROJECTD_API UPDUISoundSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="UI Sound")
	TSoftObjectPtr<USoundBase> ButtonClickSound;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="UI Sound")
	TSoftObjectPtr<USoundBase> MarketOpenSound;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="UI Sound")
	TSoftObjectPtr<USoundBase> MarketCloseSound;
};
