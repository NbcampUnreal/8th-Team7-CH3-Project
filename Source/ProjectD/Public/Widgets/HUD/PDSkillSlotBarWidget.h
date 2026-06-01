#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDSkillSlotBarWidget.generated.h"

class UTexture2D;
class UPDSkillSlotWidget;

UCLASS(BlueprintType, meta=(DisableNativeTick))
class PROJECTD_API UPDSkillSlotBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="PD|SkillSlot")
	void SetSelectedIndex(int32 NewIndex);

	UFUNCTION(BlueprintPure, Category="PD|SkillSlot")
	int32 GetSelectedIndex() const { return SelectedIndex; }

	// 슬롯에 텍스처/아이콘 푸시 + 현재 선택 상태 반영. WBP 디폴트 변경 후 재반영 시 호출.
	UFUNCTION(BlueprintCallable, Category="PD|SkillSlot")
	void RebuildSlots();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPDSkillSlotWidget> Slot_0;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPDSkillSlotWidget> Slot_1;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPDSkillSlotWidget> Slot_2;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPDSkillSlotWidget> Slot_3;

	// 인덱스별 스킬 아이콘 (4 슬롯과 같은 길이로 BP에서 채움. 비워두면 아이콘 없음)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|SkillSlot|Visuals")
	TArray<TSoftObjectPtr<UTexture2D>> SkillIcons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|SkillSlot|Visuals")
	TSoftObjectPtr<UTexture2D> SelectedTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PD|SkillSlot|Visuals")
	TSoftObjectPtr<UTexture2D> UnselectedTexture;

private:
	void CollectSlotWidgets();
	void ApplySelection();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPDSkillSlotWidget>> SlotWidgets;

	int32 SelectedIndex = INDEX_NONE;
};
