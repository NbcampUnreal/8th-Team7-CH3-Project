// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/PDHUDWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet/PDAttributeSet.h"
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
#include "Items/PDQuickSlotComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

UPDHUDWidget::UPDHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// HUD는 입력 모드를 절대 건드리면 안 됨. 베이스 기본값 Menu가 FInputModeUIOnly를 강제하는 사고 방지.
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
}

void UPDHUDWidget::NativeOnDeactivated()
{
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
		[WeakBar, WeakSelf, CurrentAttr, MaxAttr](const FOnAttributeChangeData& /*Data*/)
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

	// 초기 상태 동기화 — 바인딩 시점의 현재 태그 카운트로 핸들러 1회 호출.
	// RegisterGameplayTagEvent(NewOrRemoved)는 전이만 trigger하므로, 이미 1+ 상태인 태그를 놓치지 않기 위해 필요.
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
	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		return Pawn->FindComponentByClass<UPDQuickSlotComponent>();
	}
	return nullptr;
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

	// 이전 구독 해제 후 활성 채널링이 있었다면 시각적 잔상 제거.
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

	if (NewTarget && NewTarget->GetClass()->ImplementsInterface(UPDInteractable::StaticClass()))
	{
		const FText Display = IPDInteractable::Execute_GetInteractDisplayText(NewTarget);
		WBP_InteractPrompt->Show(Display);

		// 카메라 회전에 따라 매 프레임 위치 갱신.
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

	// 캔버스 슬롯은 슬레이트 단위(DPI 스케일 적용). 픽셀 좌표를 스케일로 나눠 변환.
	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D LocalPos = (ViewportScale > 0.f) ? (ScreenPos / ViewportScale) : ScreenPos;

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(WBP_InteractPrompt->Slot))
	{
		CanvasSlot->SetPosition(LocalPos);
	}

	// 첫 갱신에서 Hide된 상태일 수 있으므로 보장.
	if (WBP_InteractPrompt->GetVisibility() == ESlateVisibility::Collapsed
		|| WBP_InteractPrompt->GetVisibility() == ESlateVisibility::Hidden)
	{
		WBP_InteractPrompt->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

