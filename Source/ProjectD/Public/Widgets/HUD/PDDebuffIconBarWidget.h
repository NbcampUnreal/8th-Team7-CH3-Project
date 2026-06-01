// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "PDDebuffIconBarWidget.generated.h"

class UPanelWidget;
class UPDDebuffIconWidget;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct FPDDebuffIconData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UMaterialInterface> IconMaterial;
};

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDDebuffIconBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetDebuffActive(const FGameplayTag& DebuffTag, bool bActive);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPanelWidget> Container_Icons;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Debuff")
	TSubclassOf<UPDDebuffIconWidget> IconWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "PD|Debuff", meta = (ForceInlineRow))
	TMap<FGameplayTag, FPDDebuffIconData> TagToIconData;

private:
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UPDDebuffIconWidget>> ActiveIcons;
};
