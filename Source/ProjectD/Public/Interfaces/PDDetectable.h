#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PDDetectable.generated.h"

class AActor;

UINTERFACE(MinimalAPI, Blueprintable)
class UPDDetectable : public UInterface
{
	GENERATED_BODY()
};

class PROJECTD_API IPDDetectable
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, Category="PD|Vision")
	void OnVisionExposureChanged(AActor* Observer, float Exposure);
};