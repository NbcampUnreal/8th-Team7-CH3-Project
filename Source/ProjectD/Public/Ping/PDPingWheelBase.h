#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Ping/PDPingTypes.h"
#include "PDPingWheelBase.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTD_API UPDPingWheelBase : public UUserWidget
{
	GENERATED_BODY()
	
	public:
	UFUNCTION(BlueprintImplementableEvent, Category="Ping")
	void UpdateSelection(FVector2D MouseOffset);
	
	UFUNCTION(BlueprintImplementableEvent, Category="Ping")
	EPDPingType GetSelectedPingType();
	
	UFUNCTION(BlueprintImplementableEvent, Category="Ping")
	void SetWheelScreenPosition(FVector2D ScreenPos);
};
