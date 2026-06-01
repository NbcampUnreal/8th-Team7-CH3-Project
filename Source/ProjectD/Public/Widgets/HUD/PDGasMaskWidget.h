#pragma once

#include "CoreMinimal.h"
#include "Widgets/HUD/PDAttributeBarWidget.h"
#include "PDGasMaskWidget.generated.h"

class UImage;
class UTextBlock;

// 방독면 내구도 표시 위젯. 이미지 색조(초→노→빨)와 퍼센트 텍스트를 동시에 갱신.
UCLASS(BlueprintType, meta=(DisableNativeTick))
class PROJECTD_API UPDGasMaskWidget : public UPDAttributeBarWidget
{
	GENERATED_BODY()

protected:
	virtual void OnValuesUpdated(float Current, float Max, float Percent) override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UImage> Image_Mask;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_Percent;

	// Percent = 1.0일 때 색
	UPROPERTY(EditDefaultsOnly, Category="PD|GasMask|Color")
	FLinearColor ColorFull = FLinearColor::Green;

	// Percent = 0.5일 때 색
	UPROPERTY(EditDefaultsOnly, Category="PD|GasMask|Color")
	FLinearColor ColorMid = FLinearColor(1.f, 0.85f, 0.f, 1.f);

	// Percent = 0.0일 때 색
	UPROPERTY(EditDefaultsOnly, Category="PD|GasMask|Color")
	FLinearColor ColorEmpty = FLinearColor::Red;

private:
	FLinearColor ComputeColor(float Percent) const;
};