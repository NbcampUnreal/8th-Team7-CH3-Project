#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDInteractPromptWidget.generated.h"

class UImage;
class UTextBlock;
class UInputAction;
class UInputMappingContext;
class UPDKeyIconDataAsset;

UCLASS(BlueprintType, meta=(DisableNativeTick))
class PROJECTD_API UPDInteractPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="InteractPrompt")
	void Show(const FText& InText);

	UFUNCTION(BlueprintCallable, Category="InteractPrompt")
	void Hide();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UImage> Image_Key;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_Action;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="InteractPrompt")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="InteractPrompt")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="InteractPrompt")
	TSoftObjectPtr<UPDKeyIconDataAsset> KeyIconMap;

private:
	void RefreshKeyIcon();
};