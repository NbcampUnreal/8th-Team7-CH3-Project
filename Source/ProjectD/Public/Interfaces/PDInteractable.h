#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PDInteractable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPDInteractable : public UInterface
{
	GENERATED_BODY()
};

class PROJECTD_API IPDInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "PD|Interact")
	void Interact(AActor* Interactor);
};
