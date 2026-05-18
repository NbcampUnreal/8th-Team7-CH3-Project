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

	UFUNCTION(BlueprintNativeEvent, Category = "PD|Interact")
	FText GetInteractDisplayText() const;

	// 액터 메시별 위치 미세조정용. BP에서 override해 액터 머리/문/창고 윗부분 등으로 세팅.
	UFUNCTION(BlueprintNativeEvent, Category = "PD|Interact")
	FVector GetInteractPromptOffset() const;
};
