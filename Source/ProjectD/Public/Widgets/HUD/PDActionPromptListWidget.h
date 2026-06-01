#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "PDActionPromptListWidget.generated.h"

class UVerticalBox;
class UInputAction;
class UInputMappingContext;
class UPDActionPromptDataAsset;
class UPDActionPromptItemWidget;
class UPDKeyIconDataAsset;

UCLASS(BlueprintType, meta=(DisableNativeTick))
class PROJECTD_API UPDActionPromptListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="ActionPrompt")
	void RebuildPrompts();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UVerticalBox> Container_Prompts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ActionPrompt")
	TSubclassOf<UPDActionPromptItemWidget> ItemWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ActionPrompt")
	TSoftObjectPtr<UPDActionPromptDataAsset> PromptDataAsset;

	// IA → FKey 매핑이 들어있는 IMC (예: IMC_Input)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ActionPrompt")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	// FKey → 키캡 이미지 매핑 (QuickSlot과 공용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ActionPrompt")
	TSoftObjectPtr<UPDKeyIconDataAsset> KeyIconMap;

private:
	UFUNCTION()
	void HandleControlMappingsRebuilt();

	FKey FindKeyForAction(const UInputAction* Action) const;
};