#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDInventoryWeightBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

USTRUCT(BlueprintType)
struct FPDInventoryWeightBarColorRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Inventory|Weight", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinPercent = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Inventory|Weight")
	FLinearColor FillColor = FLinearColor::White;

	FPDInventoryWeightBarColorRule() = default;

	FPDInventoryWeightBarColorRule(float InMinPercent, const FLinearColor& InFillColor)
		: MinPercent(InMinPercent)
		, FillColor(InFillColor)
	{
	}
};

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDInventoryWeightBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPDInventoryWeightBarWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "PD|Inventory|Weight")
	void SetWeight(float CurrentWeight, float MaxWeight);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Inventory|Weight")
	TObjectPtr<UProgressBar> ProgressBar_Weight;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "PD|Inventory|Weight")
	TObjectPtr<UTextBlock> Text_Weight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Weight", meta = (ClampMin = "0.0"))
	float PreviewCurrentWeight = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Inventory|Weight", meta = (ClampMin = "0.0"))
	float PreviewMaxWeight = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Inventory|Weight")
	FLinearColor DefaultFillColor = FLinearColor(0.85f, 0.85f, 0.85f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Inventory|Weight")
	TArray<FPDInventoryWeightBarColorRule> ColorRules;

private:
	void RefreshWeightDisplay();
	void ApplyFillColor(const FLinearColor& FillColor);
	void EnsureDefaultColorRules();
	void AddDefaultColorRuleIfMissing(float MinPercent, const FLinearColor& FillColor);
	float GetWeightPercent() const;
	FLinearColor ResolveFillColor(float Percent) const;
	FText BuildWeightText() const;

	float CachedCurrentWeight = 0.f;
	float CachedMaxWeight = 0.f;
};
