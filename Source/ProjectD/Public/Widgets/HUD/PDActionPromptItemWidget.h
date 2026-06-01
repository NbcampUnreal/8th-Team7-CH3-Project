#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDActionPromptItemWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

UCLASS(BlueprintType, meta=(DisableNativeTick))
class PROJECTD_API UPDActionPromptItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="ActionPrompt")
	void SetPrompt(UTexture2D* InIcon, const FText& InActionText);

protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UImage> Image_Key;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_Action;
};