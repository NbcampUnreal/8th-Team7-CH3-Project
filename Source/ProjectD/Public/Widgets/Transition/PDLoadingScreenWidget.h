// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDLoadingScreenWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;
class UWidgetAnimation;


UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDLoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleLoadingReasonUpdated(const FText& InReason);

	void SetSplashImage(UTexture2D* InTexture);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> Image_Spinner;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_LoadingReason;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Splash;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetAnimOptional), Transient)
	TObjectPtr<UWidgetAnimation> Anim_SpinnerRotate;
};