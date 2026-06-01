#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDSkillSlotWidget.generated.h"

class UImage;
class UTexture2D;

UCLASS(BlueprintType, meta=(DisableNativeTick))
class PROJECTD_API UPDSkillSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="PD|SkillSlot")
	void SetSelected(bool bNewSelected);

	UFUNCTION(BlueprintCallable, Category="PD|SkillSlot")
	void SetSkillIcon(UTexture2D* InIcon);

	// BarWidget이 슬롯 생성 시 공통 텍스처를 push할 때 사용
	UFUNCTION(BlueprintCallable, Category="PD|SkillSlot")
	void SetSlotTextures(TSoftObjectPtr<UTexture2D> InSelectedTex, TSoftObjectPtr<UTexture2D> InUnselectedTex);

	UFUNCTION(BlueprintPure, Category="PD|SkillSlot")
	bool IsSlotSelected() const { return bSelected; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UImage> Image_SlotBG;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UImage> Image_SkillIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|SkillSlot|Visuals")
	TSoftObjectPtr<UTexture2D> SelectedTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|SkillSlot|Visuals")
	TSoftObjectPtr<UTexture2D> UnselectedTexture;

private:
	bool bSelected = false;
};