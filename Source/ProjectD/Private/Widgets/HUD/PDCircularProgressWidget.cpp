#include "Widgets/HUD/PDCircularProgressWidget.h"

#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/PDKeyIconDataAsset.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Materials/MaterialInstanceDynamic.h"

void UPDCircularProgressWidget::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureMID();
	SetProgressScalar(0.f);
	SetVisibility(ESlateVisibility::Collapsed);

	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Sub->ControlMappingsRebuiltDelegate.AddUniqueDynamic(this, &UPDCircularProgressWidget::HandleControlMappingsRebuilt);
		}
	}

	RefreshCancelHint();
}

void UPDCircularProgressWidget::NativeDestruct()
{
	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Sub->ControlMappingsRebuiltDelegate.RemoveDynamic(this, &UPDCircularProgressWidget::HandleControlMappingsRebuilt);
		}
	}

	Super::NativeDestruct();
}

void UPDCircularProgressWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bExternalDriven)
	{
		return;
	}

	if (!bRunning)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World || Duration <= 0.f)
	{
		return;
	}

	const float Elapsed = World->GetTimeSeconds() - StartTime;
	const float Progress = FMath::Clamp(Elapsed / Duration, 0.f, 1.f);
	SetProgressScalar(Progress);
	UpdateRemainingText(FMath::Max(Duration - Elapsed, 0.f));

	if (Progress >= 1.f)
	{
		CompleteProgress();
	}
}

void UPDCircularProgressWidget::StartProgress(float InDuration)
{
	if (InDuration <= 0.f)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	EnsureMID();

	Duration = InDuration;
	StartTime = World->GetTimeSeconds();
	bRunning = true;
	bExternalDriven = false;

	SetProgressScalar(0.f);
	UpdateRemainingText(Duration);
	RefreshCancelHint();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UPDCircularProgressWidget::StopProgress()
{
	bRunning = false;
	bExternalDriven = false;
	Duration = 0.f;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPDCircularProgressWidget::CompleteProgress()
{
	bRunning = false;
	bExternalDriven = false;
	SetProgressScalar(1.f);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPDCircularProgressWidget::StartExternalDriven()
{
	EnsureMID();

	bRunning = false;
	bExternalDriven = true;
	Duration = 0.f;

	SetProgressScalar(0.f);
	RefreshCancelHint();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UPDCircularProgressWidget::PushExternalProgress(float Progress01, float OptionalRemainingSeconds)
{
	if (!bExternalDriven)
	{
		return;
	}

	SetProgressScalar(FMath::Clamp(Progress01, 0.f, 1.f));

	if (OptionalRemainingSeconds >= 0.f)
	{
		UpdateRemainingText(OptionalRemainingSeconds);
	}
}

void UPDCircularProgressWidget::EnsureMID()
{
	if (RingMID || !Image_Ring)
	{
		return;
	}
	// GetDynamicMaterial: 이미 MID면 그대로 반환, 아니면 Brush 머터리얼로부터 생성해 박는다.
	RingMID = Image_Ring->GetDynamicMaterial();
}

void UPDCircularProgressWidget::SetProgressScalar(float Value)
{
	if (RingMID)
	{
		RingMID->SetScalarParameterValue(ProgressParamName, Value);
	}
}

void UPDCircularProgressWidget::UpdateRemainingText(float RemainingSeconds)
{
	if (!Text_Remaining)
	{
		return;
	}
	Text_Remaining->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), RemainingSeconds)));
}

void UPDCircularProgressWidget::RefreshCancelHint()
{
	if (!Box_CancelHint)
	{
		return;
	}

	if (!CancelInputAction)
	{
		Box_CancelHint->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	UPDKeyIconDataAsset* IconMap = KeyIconMap.LoadSynchronous();
	const FKey Key = FindKeyForAction(CancelInputAction);
	UTexture2D* Icon = (IconMap && Key.IsValid()) ? IconMap->ResolveIcon(Key) : nullptr;

	if (Image_CancelKeyIcon)
	{
		if (Icon)
		{
			Image_CancelKeyIcon->SetBrushFromTexture(Icon);
			Image_CancelKeyIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Image_CancelKeyIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (Text_CancelLabel)
	{
		Text_CancelLabel->SetText(CancelLabelText);
	}

	Box_CancelHint->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPDCircularProgressWidget::HandleControlMappingsRebuilt()
{
	RefreshCancelHint();
}

FKey UPDCircularProgressWidget::FindKeyForAction(const UInputAction* Action) const
{
	if (!Action || !InputMappingContext)
	{
		return FKey();
	}

	for (const FEnhancedActionKeyMapping& Mapping : InputMappingContext->GetMappings())
	{
		if (Mapping.Action == Action)
		{
			return Mapping.Key;
		}
	}

	return FKey();
}