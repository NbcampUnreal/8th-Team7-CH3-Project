#include "Widgets/HUD/PDGasMaskWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void UPDGasMaskWidget::OnValuesUpdated(float Current, float Max, float Percent)
{
	if (Image_Mask)
	{
		Image_Mask->SetColorAndOpacity(ComputeColor(Percent));
	}

	if (Text_Percent)
	{
		const int32 Pct = FMath::RoundToInt(FMath::Clamp(Percent, 0.f, 1.f) * 100.f);
		Text_Percent->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), Pct)));
	}
}

FLinearColor UPDGasMaskWidget::ComputeColor(float Percent) const
{
	Percent = FMath::Clamp(Percent, 0.f, 1.f);
	if (Percent >= 0.5f)
	{
		const float T = (Percent - 0.5f) * 2.f;
		return FMath::Lerp(ColorMid, ColorFull, T);
	}

	const float T = Percent * 2.f;
	return FMath::Lerp(ColorEmpty, ColorMid, T);
}