


#include "Widgets/HUD/PDHUDWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet/PDAttributeSet.h"
#include "Core/PDPlayerComponentResolver.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Type/Types.h"
#include "Widgets/HUD/PDActionPromptListWidget.h"
#include "Widgets/HUD/PDAttributeBarWidget.h"
#include "Widgets/HUD/PDBodyPartHealthGroupWidget.h"
#include "Widgets/HUD/PDCircularProgressWidget.h"
#include "Widgets/HUD/PDGasMaskWidget.h"
#include "Widgets/HUD/PDInteractPromptWidget.h"
#include "Widgets/HUD/PDNewQuickSlotBarWidget.h"
#include "Widgets/HUD/PDDebuffIconBarWidget.h"
#include "Widgets/HUD/PDSkillSlotBarWidget.h"
#include "Widgets/Crosshair/PDCrosshairWidget.h"
#include "Component/PDInteractionComponent.h"
#include "Interfaces/PDInteractable.h"
#include "Items/Containers/PDQuickSlotComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

UPDHUDWidget::UPDHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	InputMode = EWidgetInputMode::Passive;
}

void UPDHUDWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	APawn* Pawn = GetOwningPlayerPawn();
	UAbilitySystemComponent* ASC = Pawn
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn)
		: nullptr;
	RebindToASC(ASC);

	RefreshNewQuickSlots();
	RefreshInteractPromptBinding(FindOwningInteractionComponent());

	BindSpectatorDelegate(Cast<APDPlayerController>(GetOwningPlayer()));
	RefreshSpectateDisplay();
}

void UPDHUDWidget::NativeOnDeactivated()
{
	UnbindSpectatorDelegate();
	RefreshInteractPromptBinding(nullptr);
	RefreshUseProgressBinding(nullptr);
	RebindToASC(nullptr);
	Super::NativeOnDeactivated();
}

void UPDHUDWidget::RebindToASC(UAbilitySystemComponent* NewASC)
{
	if (CachedASC.Get() == NewASC) return;

	UnbindAllAttributes();
	UnbindAllTags();

	CachedASC = NewASC;
	if (!NewASC) return;

	BindAllAttributes();
	BindAllDebuffTags();
}

void UPDHUDWidget::BindAllAttributes()
{
	BindAttributeToBar(Bar_Stamina,
		UPDAttributeSet::GetStaminaAttribute(),
		UPDAttributeSet::GetMaxStaminaAttribute());

	BindAttributeToBar(Bar_Hunger,
		UPDAttributeSet::GetHungerAttribute(),
		UPDAttributeSet::GetMaxHungerAttribute());

	BindAttributeToBar(Bar_Thirst,
		UPDAttributeSet::GetThirstAttribute(),
		UPDAttributeSet::GetMaxThirstAttribute());

	BindAttributeToBar(Bar_GasMask,
		UPDAttributeSet::GetGasMaskAttribute(),
		UPDAttributeSet::GetMaxGasMaskAttribute());

	if (Bar_BodyParts)
	{
		BindAttributeToBar(Bar_BodyParts->GetBar(EBodyPart::Head),
			UPDAttributeSet::GetHeadHPAttribute(),
			UPDAttributeSet::GetMaxHeadHPAttribute());

		BindAttributeToBar(Bar_BodyParts->GetBar(EBodyPart::Torso),
			UPDAttributeSet::GetTorsoHPAttribute(),
			UPDAttributeSet::GetMaxTorsoHPAttribute());

		BindAttributeToBar(Bar_BodyParts->GetBar(EBodyPart::Arm_L),
			UPDAttributeSet::GetArmLHPAttribute(),
			UPDAttributeSet::GetMaxArmLHPAttribute());

		BindAttributeToBar(Bar_BodyParts->GetBar(EBodyPart::Arm_R),
			UPDAttributeSet::GetArmRHPAttribute(),
			UPDAttributeSet::GetMaxArmRHPAttribute());

		BindAttributeToBar(Bar_BodyParts->GetBar(EBodyPart::Leg_L),
			UPDAttributeSet::GetLegLHPAttribute(),
			UPDAttributeSet::GetMaxLegLHPAttribute());

		BindAttributeToBar(Bar_BodyParts->GetBar(EBodyPart::Leg_R),
			UPDAttributeSet::GetLegRHPAttribute(),
			UPDAttributeSet::GetMaxLegRHPAttribute());
	}
}

void UPDHUDWidget::BindAttributeToBar(UPDAttributeBarWidget* Bar,
	const FGameplayAttribute& CurrentAttr,
	const FGameplayAttribute& MaxAttr)
{
	if (!Bar || !CachedASC.IsValid()) return;

	RefreshBar(Bar, CurrentAttr, MaxAttr);

	UAbilitySystemComponent* ASC = CachedASC.Get();
	TWeakObjectPtr<UPDAttributeBarWidget> WeakBar = Bar;
	TWeakObjectPtr<UPDHUDWidget> WeakSelf = this;

	auto OnChanged =
		[WeakBar, WeakSelf, CurrentAttr, MaxAttr](const FOnAttributeChangeData& )
		{
			if (!WeakBar.IsValid() || !WeakSelf.IsValid()) return;
			WeakSelf->RefreshBar(WeakBar.Get(), CurrentAttr, MaxAttr);
		};

	BoundAttributeHandles.Add({ CurrentAttr,
		ASC->GetGameplayAttributeValueChangeDelegate(CurrentAttr).AddLambda(OnChanged) });

	BoundAttributeHandles.Add({ MaxAttr,
		ASC->GetGameplayAttributeValueChangeDelegate(MaxAttr).AddLambda(OnChanged) });
}

void UPDHUDWidget::RefreshBar(UPDAttributeBarWidget* Bar,
	const FGameplayAttribute& CurrentAttr,
	const FGameplayAttribute& MaxAttr) const
{
	if (!Bar || !CachedASC.IsValid()) return;

	UAbilitySystemComponent* ASC = CachedASC.Get();
	Bar->SetValues(
		ASC->GetNumericAttribute(CurrentAttr),
		ASC->GetNumericAttribute(MaxAttr));
}

void UPDHUDWidget::UnbindAllAttributes()
{
	if (CachedASC.IsValid())
	{
		UAbilitySystemComponent* ASC = CachedASC.Get();
		for (const FBoundAttributeHandle& BH : BoundAttributeHandles)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(BH.Attribute).Remove(BH.Handle);
		}
	}
	BoundAttributeHandles.Reset();
}

void UPDHUDWidget::BindAllDebuffTags()
{
	BindTagEvent(PDGameplayTags::State_Debuff_Bleeding,
		[this](const FGameplayTag& Tag, int32 NewCount) { HandleBleeding(Tag, NewCount); });

	BindTagEvent(PDGameplayTags::State_Debuff_LegDamaged,
		[this](const FGameplayTag& Tag, int32 NewCount) { HandleLegDamaged(Tag, NewCount); });

	BindTagEvent(PDGameplayTags::State_Debuff_ArmDamaged,
		[this](const FGameplayTag& Tag, int32 NewCount) { HandleArmDamaged(Tag, NewCount); });

	BindTagEvent(PDGameplayTags::State_Debuff_Starving,
		[this](const FGameplayTag& Tag, int32 NewCount) { HandleStarving(Tag, NewCount); });

	BindTagEvent(PDGameplayTags::State_Debuff_Dehydrated,
		[this](const FGameplayTag& Tag, int32 NewCount) { HandleDehydrated(Tag, NewCount); });
}

void UPDHUDWidget::BindTagEvent(const FGameplayTag& Tag,
	TFunction<void(const FGameplayTag&, int32)> OnChanged)
{
	if (!CachedASC.IsValid()) return;
	UAbilitySystemComponent* ASC = CachedASC.Get();



	OnChanged(Tag, ASC->GetGameplayTagCount(Tag));

	TWeakObjectPtr<UPDHUDWidget> WeakSelf = this;
	FDelegateHandle Handle = ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
		.AddLambda([WeakSelf, OnChanged](const FGameplayTag CallbackTag, int32 NewCount)
		{
			if (!WeakSelf.IsValid()) return;
			OnChanged(CallbackTag, NewCount);
		});

	BoundTagHandles.Add({ Tag, Handle });
}

void UPDHUDWidget::UnbindAllTags()
{
	if (CachedASC.IsValid())
	{
		UAbilitySystemComponent* ASC = CachedASC.Get();
		for (const FBoundTagHandle& BH : BoundTagHandles)
		{
			ASC->RegisterGameplayTagEvent(BH.Tag, EGameplayTagEventType::NewOrRemoved).Remove(BH.Handle);
		}
	}
	BoundTagHandles.Reset();
}

void UPDHUDWidget::HandleBleeding(const FGameplayTag& Tag, int32 NewCount)
{
	if (Bar_Debuffs)
	{
		Bar_Debuffs->SetDebuffActive(Tag, NewCount > 0);
	}
}

void UPDHUDWidget::HandleLegDamaged(const FGameplayTag& Tag, int32 NewCount)
{
	if (Bar_Debuffs)
	{
		Bar_Debuffs->SetDebuffActive(Tag, NewCount > 0);
	}
}

void UPDHUDWidget::HandleArmDamaged(const FGameplayTag& Tag, int32 NewCount)
{
	if (Bar_Debuffs)
	{
		Bar_Debuffs->SetDebuffActive(Tag, NewCount > 0);
	}
}

void UPDHUDWidget::HandleStarving(const FGameplayTag& Tag, int32 NewCount)
{
	if (Bar_Debuffs)
	{
		Bar_Debuffs->SetDebuffActive(Tag, NewCount > 0);
	}
}

void UPDHUDWidget::HandleDehydrated(const FGameplayTag& Tag, int32 NewCount)
{
	if (Bar_Debuffs)
	{
		Bar_Debuffs->SetDebuffActive(Tag, NewCount > 0);
	}
}

void UPDHUDWidget::UpdateCrosshair(FVector2D MousePos, float Spread)
{
	if (WBP_Crosshair) WBP_Crosshair->UpdateCrosshair(MousePos, Spread);
}

void UPDHUDWidget::SetCrosshairType(EWeaponType NewType)
{
	if (WBP_Crosshair) WBP_Crosshair->SetCrosshairType(NewType);
}

void UPDHUDWidget::SetCrosshairVisible(bool bVisible)
{
	if (!WBP_Crosshair) return;
	WBP_Crosshair->SetVisibility(
		bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UPDHUDWidget::RefreshNewQuickSlots()
{
	UPDQuickSlotComponent* QuickSlotComponent = FindOwningQuickSlotComponent();

	if (Bar_NewQuickSlots)
	{
		Bar_NewQuickSlots->BindQuickSlotComponent(QuickSlotComponent);
	}

	RefreshUseProgressBinding(QuickSlotComponent);
}

UPDQuickSlotComponent* UPDHUDWidget::FindOwningQuickSlotComponent() const
{
	if (UPDQuickSlotComponent* QuickSlot = FPDPlayerComponentResolver::ResolveQuickSlot(GetOwningPlayer()))
	{
		return QuickSlot;
	}
	return FPDPlayerComponentResolver::ResolveQuickSlot(GetOwningPlayerPawn());
}

void UPDHUDWidget::RefreshUseProgressBinding(UPDQuickSlotComponent* NewComponent)
{
	UPDQuickSlotComponent* Old = CachedQuickSlot.Get();
	if (Old == NewComponent)
	{
		return;
	}

	if (Old)
	{
		Old->OnConsumableUseStarted.RemoveDynamic(this, &UPDHUDWidget::HandleConsumableUseStarted);
		Old->OnConsumableUseCanceled.RemoveDynamic(this, &UPDHUDWidget::HandleConsumableUseCanceled);
		Old->OnConsumableUseCompleted.RemoveDynamic(this, &UPDHUDWidget::HandleConsumableUseCompleted);
	}


	if (WBP_UseProgress && WBP_UseProgress->IsRunning())
	{
		WBP_UseProgress->StopProgress();
	}

	CachedQuickSlot = NewComponent;

	if (NewComponent)
	{
		NewComponent->OnConsumableUseStarted.AddUniqueDynamic(this, &UPDHUDWidget::HandleConsumableUseStarted);
		NewComponent->OnConsumableUseCanceled.AddUniqueDynamic(this, &UPDHUDWidget::HandleConsumableUseCanceled);
		NewComponent->OnConsumableUseCompleted.AddUniqueDynamic(this, &UPDHUDWidget::HandleConsumableUseCompleted);
	}
}

void UPDHUDWidget::HandleConsumableUseStarted(int32 SlotIndex, FPDItemData ItemData, float Duration)
{
	if (WBP_UseProgress)
	{
		WBP_UseProgress->StartProgress(Duration);
	}
}

void UPDHUDWidget::HandleConsumableUseCanceled(int32 SlotIndex, FPDItemData ItemData)
{
	if (WBP_UseProgress)
	{
		WBP_UseProgress->StopProgress();
	}
}

void UPDHUDWidget::HandleConsumableUseCompleted(int32 SlotIndex, FPDItemData ItemData)
{
	if (WBP_UseProgress)
	{
		WBP_UseProgress->CompleteProgress();
	}
}

void UPDHUDWidget::StartReviveProgress(AActor* Target, float Duration)
{
	if (!WBP_ReviveProgress || !Target) return;

	CachedReviveTarget = Target;
	bSuppressInteractPrompt = true;

	// InteractPrompt 강제 숨김 + 갱신 타이머 정지 — Revive 위젯이 자리를 차지.
	if (WBP_InteractPrompt) WBP_InteractPrompt->Hide();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InteractPromptUpdateTimer);
	}

	WBP_ReviveProgress->StartProgress(Duration);

	UpdateReviveProgressPosition();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ReviveProgressUpdateTimer,
			this,
			&UPDHUDWidget::UpdateReviveProgressPosition,
			1.f / 60.f,
			true);
	}
}

void UPDHUDWidget::StopReviveProgress()
{
	if (WBP_ReviveProgress) WBP_ReviveProgress->StopProgress();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReviveProgressUpdateTimer);
	}
	CachedReviveTarget = nullptr;
	bSuppressInteractPrompt = false;
	// InteractPrompt 는 다음 InteractionComponent poll 에서 자연 복귀 (대상 여전히 유효시).
}

void UPDHUDWidget::CompleteReviveProgress()
{
	if (WBP_ReviveProgress) WBP_ReviveProgress->CompleteProgress();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReviveProgressUpdateTimer);
	}
	CachedReviveTarget = nullptr;
	bSuppressInteractPrompt = false;
}

void UPDHUDWidget::UpdateReviveProgressPosition()
{
	if (!WBP_ReviveProgress) return;
	AActor* Target = CachedReviveTarget.Get();
	if (!Target)
	{
		StopReviveProgress();
		return;
	}
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	const FVector WorldLoc = Target->GetActorLocation() + ReviveProgressWorldOffset;
	FVector2D ScreenPos;
	if (!PC->ProjectWorldLocationToScreen(WorldLoc, ScreenPos))
	{
		WBP_ReviveProgress->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D LocalPos = (ViewportScale > 0.f) ? (ScreenPos / ViewportScale) : ScreenPos;

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(WBP_ReviveProgress->Slot))
	{
		CanvasSlot->SetPosition(LocalPos);
	}

	if (WBP_ReviveProgress->GetVisibility() == ESlateVisibility::Hidden)
	{
		WBP_ReviveProgress->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

UPDInteractionComponent* UPDHUDWidget::FindOwningInteractionComponent() const
{
	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		return Pawn->FindComponentByClass<UPDInteractionComponent>();
	}
	return nullptr;
}

void UPDHUDWidget::RefreshInteractPromptBinding(UPDInteractionComponent* NewComponent)
{
	UPDInteractionComponent* Old = CachedInteraction.Get();
	if (Old == NewComponent)
	{
		return;
	}

	if (Old)
	{
		Old->OnInteractTargetChanged.RemoveDynamic(this, &UPDHUDWidget::HandleInteractTargetChanged);
	}

	if (WBP_InteractPrompt)
	{
		WBP_InteractPrompt->Hide();
	}
	CachedInteractTarget = nullptr;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InteractPromptUpdateTimer);
	}

	CachedInteraction = NewComponent;

	if (NewComponent)
	{
		NewComponent->OnInteractTargetChanged.AddUniqueDynamic(this, &UPDHUDWidget::HandleInteractTargetChanged);
		HandleInteractTargetChanged(NewComponent->GetCurrentTarget());
	}
}

void UPDHUDWidget::HandleInteractTargetChanged(AActor* NewTarget)
{
	if (!WBP_InteractPrompt)
	{
		return;
	}

	CachedInteractTarget = NewTarget;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Revive 진행 중에는 InteractPrompt 표시 억제 — ReviveProgress 위젯이 자리 차지.
	if (bSuppressInteractPrompt)
	{
		WBP_InteractPrompt->Hide();
		World->GetTimerManager().ClearTimer(InteractPromptUpdateTimer);
		return;
	}

	if (NewTarget && NewTarget->GetClass()->ImplementsInterface(UPDInteractable::StaticClass()))
	{
		const FText Display = IPDInteractable::Execute_GetInteractDisplayText(NewTarget);
		WBP_InteractPrompt->Show(Display);

		// 카메???�전???�라 �??�레???�치 갱신.
		UpdateInteractPromptPosition();
		World->GetTimerManager().SetTimer(
			InteractPromptUpdateTimer,
			this,
			&UPDHUDWidget::UpdateInteractPromptPosition,
			1.f / 60.f,
			true);
	}
	else
	{
		WBP_InteractPrompt->Hide();
		World->GetTimerManager().ClearTimer(InteractPromptUpdateTimer);
	}
}

void UPDHUDWidget::UpdateInteractPromptPosition()
{
	if (!WBP_InteractPrompt)
	{
		return;
	}

	AActor* Target = CachedInteractTarget.Get();
	if (!Target)
	{
		WBP_InteractPrompt->Hide();
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(InteractPromptUpdateTimer);
		}
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	FVector Offset = FVector::ZeroVector;
	if (Target->GetClass()->ImplementsInterface(UPDInteractable::StaticClass()))
	{
		Offset = IPDInteractable::Execute_GetInteractPromptOffset(Target);
	}
	const FVector WorldLoc = Target->GetActorLocation() + Offset;

	FVector2D ScreenPos;
	const bool bOnScreen = PC->ProjectWorldLocationToScreen(WorldLoc, ScreenPos);

	if (!bOnScreen)
	{
		WBP_InteractPrompt->Hide();
		return;
	}

	// 캔버???�롯?� ?�레?�트 ?�위(DPI ?��????�용). ?��? 좌표�??��??�로 ?�눠 변??
	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D LocalPos = (ViewportScale > 0.f) ? (ScreenPos / ViewportScale) : ScreenPos;

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(WBP_InteractPrompt->Slot))
	{
		CanvasSlot->SetPosition(LocalPos);
	}

	// �?갱신?�서 Hide???�태?????�으므�?보장.
	if (WBP_InteractPrompt->GetVisibility() == ESlateVisibility::Collapsed
		|| WBP_InteractPrompt->GetVisibility() == ESlateVisibility::Hidden)
	{
		WBP_InteractPrompt->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UPDHUDWidget::BindSpectatorDelegate(APDPlayerController* PC)
{
	if (CachedSpectatorPC.Get() == PC) return;

	UnbindSpectatorDelegate();
	if (!PC) return;

	PC->OnSpectateChanged.AddDynamic(this, &UPDHUDWidget::HandleSpectateChanged);
	CachedSpectatorPC = PC;
}

void UPDHUDWidget::UnbindSpectatorDelegate()
{
	if (APDPlayerController* PC = CachedSpectatorPC.Get())
	{
		PC->OnSpectateChanged.RemoveDynamic(this, &UPDHUDWidget::HandleSpectateChanged);
	}
	CachedSpectatorPC.Reset();
}

void UPDHUDWidget::HandleSpectateChanged()
{
	RefreshSpectateDisplay();
}

void UPDHUDWidget::RefreshSpectateDisplay()
{
	if (!Text_SpectateTarget) return;

	APDPlayerController* PC = CachedSpectatorPC.Get();
	if (!PC || !PC->IsSpectating())
	{
		Text_SpectateTarget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FString TargetName = PC->GetSpectateTargetName();
	if (TargetName.IsEmpty())
	{
		Text_SpectateTarget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	Text_SpectateTarget->SetText(FText::FromString(FString::Printf(TEXT("Spectating: %s"), *TargetName)));
	Text_SpectateTarget->SetVisibility(ESlateVisibility::HitTestInvisible);
}
