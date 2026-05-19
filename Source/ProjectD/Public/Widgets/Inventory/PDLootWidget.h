#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDLootWidget.generated.h"

class UPDLootComponent;
class UPDInventorySlotWidget;
class UUniformGridPanel;
class UUserWidget;
class USizeBox;

/**
 * LootBox 전용 위젯.
 *  - UPDStashWidget 와 코드/상속 무관 (별도 클래스 계층).
 *  - 분류/탭/업그레이드/정렬/드래그앤드롭 UI 없음 — 슬롯 그리드 + Left-click Take 만 지원.
 *  - WBP_Loot 의 위젯 트리에 LootGridWidgetName(기본 "UniformGridPanel_LootGrid") 으로
 *    UniformGridPanel 배치 + Class Defaults 의 LootSlotWidgetClass 지정으로 동작.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDLootWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	void InitializeLoot(UPDLootComponent* InLootComponent);

	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	void RefreshLootGrid();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot")
	TSubclassOf<UUserWidget> LootSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot")
	FName LootGridWidgetName = TEXT("UniformGridPanel_LootGrid");

	/** 그리드 패널 이름 검색 실패 시 사용할 fallback — 위젯 트리에서 아무 UniformGridPanel 자동 탐색 여부. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot")
	bool bUseGridPanelFallback = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot")
	float SlotWidth = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot")
	float SlotHeight = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot")
	int32 FallbackGridColumns = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot")
	int32 FallbackGridRows = 4;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "PD|Loot")
	TObjectPtr<UUniformGridPanel> LootGridPanel;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "PD|Loot")
	TObjectPtr<UPDLootComponent> TargetLootComponent;

	UFUNCTION()
	void HandleLootSlotLeftClicked(UPDInventorySlotWidget* SlotWidget, int32 ClickedSlotIndex);

private:
	void ResolveGridPanel();
	void BindLootChanged();
	void UnbindLootChanged();

	UPROPERTY(Transient)
	TObjectPtr<UPDLootComponent> BoundLootComponent;
};
