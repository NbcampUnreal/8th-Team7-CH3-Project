#include "Widgets/Inventory/PDInventoryWeightBarWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Styling/SlateColor.h"

UPDInventoryWeightBarWidget::UPDInventoryWeightBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPDInventoryWeightBarWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsDesignTime())
	{
		CachedCurrentWeight = FMath::Max(0.f, PreviewCurrentWeight);
		CachedMaxWeight = FMath::Max(0.f, PreviewMaxWeight);
	}

	RefreshWeightDisplay();
}

void UPDInventoryWeightBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshWeightDisplay();
}

void UPDInventoryWeightBarWidget::SetWeight(float CurrentWeight, float MaxWeight)
{
	CachedCurrentWeight = FMath::Max(0.f, CurrentWeight);
	CachedMaxWeight = FMath::Max(0.f, MaxWeight);

	RefreshWeightDisplay();
}

void UPDInventoryWeightBarWidget::RefreshWeightDisplay()
{
	const float Percent = GetWeightPercent();

	if (ProgressBar_Weight)
	{
		ProgressBar_Weight->SetPercent(Percent);
		ApplyFillColor(ResolveFillColor(Percent));
	}

	if (Text_Weight)
	{
		Text_Weight->SetText(BuildWeightText());
	}
}

void UPDInventoryWeightBarWidget::ApplyFillColor(const FLinearColor& FillColor)
{
	if (!ProgressBar_Weight)
	{
		return;
	}

	FProgressBarStyle ProgressBarStyle = ProgressBar_Weight->GetWidgetStyle();
	ProgressBarStyle.FillImage.TintColor = FSlateColor(FillColor);
	ProgressBar_Weight->SetWidgetStyle(ProgressBarStyle);
	ProgressBar_Weight->SetFillColorAndOpacity(FillColor);
}

float UPDInventoryWeightBarWidget::GetWeightPercent() const
{
	if (CachedMaxWeight <= 0.f)
	{
		return 0.f;
	}

	return FMath::Clamp(CachedCurrentWeight / CachedMaxWeight, 0.f, 1.f);
}

FLinearColor UPDInventoryWeightBarWidget::ResolveFillColor(float Percent) const
{
	return Percent >= 0.9f ? FLinearColor::Red : DefaultFillColor;
}

FText UPDInventoryWeightBarWidget::BuildWeightText() const
{
	return FText::FromString(FString::Printf(TEXT("%.1f / %.1fkg"), CachedCurrentWeight, CachedMaxWeight));
}