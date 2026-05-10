// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDAttributeBarWidget.generated.h"

/**
 * GAS Attribute의 Current/Max 값을 받아 표시하는 추상 베이스.
 * 시각화는 서브클래스가 OnValuesUpdated 를 오버라이드해 결정한다.
 * ASC를 직접 참조하지 않음 — 상위 HUD가 push해 준다.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDAttributeBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|HUD")
	void SetValues(float Current, float Max);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PD|LinearAttributeBar")
	FString TargetText{FString("")};

protected:
	virtual void OnValuesUpdated(float Current, float Max, float Percent) {}
};
