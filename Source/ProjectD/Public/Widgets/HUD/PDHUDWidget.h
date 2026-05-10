// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "AttributeSet.h"
#include "PDHUDWidget.generated.h"

class UPDAttributeBarWidget;
class UPDBodyPartHealthGroupWidget;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;

/**
 * 플레이어 HUD 위젯
 * Activate 시점에 Pawn의 ASC를 잡아 attribute 변경 델리게이트를 구독하고, Deactivate 시점에 구독을 해제
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDHUDWidget : public UPDActivatableBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPDBodyPartHealthGroupWidget> Bar_BodyParts;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPDAttributeBarWidget> Bar_Stamina;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPDAttributeBarWidget> Bar_Hunger;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPDAttributeBarWidget> Bar_Thirst;

private:
	struct FBoundHandle
	{
		FGameplayAttribute Attribute;
		FDelegateHandle Handle;
	};

	void TryBindToOwningPawnASC();

	void BindAttributeToBar(UPDAttributeBarWidget* Bar,
		const FGameplayAttribute& CurrentAttr,
		const FGameplayAttribute& MaxAttr);

	void RefreshBar(UPDAttributeBarWidget* Bar,
		const FGameplayAttribute& CurrentAttr,
		const FGameplayAttribute& MaxAttr) const;

	void UnbindAll();

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	TArray<FBoundHandle> BoundHandles;
};
