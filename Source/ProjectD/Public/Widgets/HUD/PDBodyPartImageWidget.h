// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/HUD/PDAttributeBarWidget.h"
#include "PDBodyPartImageWidget.generated.h"

class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * 부위별 신체 마스크 이미지 위젯
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDBodyPartImageWidget : public UPDAttributeBarWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void OnValuesUpdated(float Current, float Max, float Percent) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> Image_Part;

	// 이 인스턴스에서 사용할 MI
	UPROPERTY(EditAnywhere, Category = "PD|HUD")
	TObjectPtr<UMaterialInterface> PartMaterial;

	// 마스터 머터리얼의 fill 스칼라 파라미터 이름
	UPROPERTY(EditDefaultsOnly, Category = "PD|HUD")
	FName PercentParamName = TEXT("Percent");

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PartMID;
};