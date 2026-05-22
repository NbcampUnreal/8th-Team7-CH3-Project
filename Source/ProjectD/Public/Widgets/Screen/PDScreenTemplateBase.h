#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDScreenTemplateBase.generated.h"

class UNamedSlot;
class UUserWidget;
class UWidget;

/**
 * Tarkov 스타일 공통 chrome을 제공하는 화면 베이스.
 * NamedSlot: MainLeft / MainRight / Description / Modal (+ 파생 클래스가 추가)
 *
 * 책임:
 *  - Description 슬롯 텍스트 갱신 / 빈 텍스트면 Collapsed
 *  - Modal sub-widget 마운트/언마운트
 *  - ESC 입력 → HandleEscape (기본은 닫기, override 가능)
 *
 * 탭 로직은 UPDTabbedScreenBase에서 추가된다. 비탭 화면은 이 클래스를 직접 상속해도 된다.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDScreenTemplateBase : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	/** Description 슬롯 갱신. 빈 텍스트 → 슬롯 Collapsed. */
	UFUNCTION(BlueprintCallable, Category = "PD|Screen")
	void SetDescription(const FText& NewDescription);

	/** 모달 위젯을 Modal 슬롯에 마운트. 이미 떠있던 모달은 먼저 Dismiss. */
	UFUNCTION(BlueprintCallable, Category = "PD|Screen|Modal")
	void ShowModal(UUserWidget* ModalWidget);

	/** 현재 마운트된 모달 제거. */
	UFUNCTION(BlueprintCallable, Category = "PD|Screen|Modal")
	void DismissModal();

	/** 컨텐츠 위젯이 부모 체인에서 가장 가까운 ScreenTemplate를 찾을 때 사용. nullptr 가능. */
	UFUNCTION(BlueprintCallable, Category = "PD|Screen", meta = (DefaultToSelf = "Source"))
	static UPDScreenTemplateBase* FindForWidget(UWidget* Source);

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** ESC 등 닫기 키 입력 시 호출. true 반환 시 이벤트 소비. 기본은 화면 pop 후 true. */
	UFUNCTION(BlueprintNativeEvent, Category = "PD|Screen")
	bool HandleEscape();
	virtual bool HandleEscape_Implementation();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNamedSlot> NamedSlot_Description;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UNamedSlot> NamedSlot_Modal;

private:
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveModalWidget;
};