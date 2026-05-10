// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Type/Types.h"
#include "PDBodyPartHealthGroupWidget.generated.h"

class UPDAttributeBarWidget;

/**
 * 6부위(Head/Torso/Arm_L/Arm_R/Leg_L/Leg_R) HP 바를 묶는 합성 위젯.
 * ASC를 직접 참조하지 않음 — 상위 HUD가 GetBar(Part)로 개별 바를 가져가 push.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDBodyPartHealthGroupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|HUD")
	UPDAttributeBarWidget* GetBar(EBodyPart Part) const;

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPDAttributeBarWidget> Bar_Head;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPDAttributeBarWidget> Bar_Torso;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPDAttributeBarWidget> Bar_ArmL;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPDAttributeBarWidget> Bar_ArmR;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPDAttributeBarWidget> Bar_LegL;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPDAttributeBarWidget> Bar_LegR;
};
