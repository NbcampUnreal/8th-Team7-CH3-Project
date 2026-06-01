#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PDWidgetSoundLibrary.generated.h"

class USoundBase;

UCLASS()
class PROJECTD_API UPDWidgetSoundLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|UI Sound", meta = (WorldContext = "WorldContextObject"))
	static void PlayUISound2D(const UObject* WorldContextObject, USoundBase* Sound);
};
