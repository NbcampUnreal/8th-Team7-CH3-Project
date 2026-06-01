#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDPingIconWidget.generated.h"

class UTexture2D;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API UPDPingIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category="Ping")
	int32 PingId = -1;

	UPROPERTY(BlueprintReadWrite, Category="Ping")
	FVector WorldLocation = FVector::ZeroVector;

	UFUNCTION(BlueprintImplementableEvent, Category="Ping")
	void SetIconTexture(UTexture2D* InTexture);
};