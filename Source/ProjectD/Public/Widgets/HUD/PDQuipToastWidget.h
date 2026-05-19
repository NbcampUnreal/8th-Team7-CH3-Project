#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDQuipToastWidget.generated.h"

class UTextBlock;
class UBorder;
class UWidgetAnimation;
class UPDQuipSubsystem;
struct FPDQuipEntry;

/**
 * 캐릭터 Quip(멘트) 표시 위젯. 1줄 교체 모드.
 * UPDQuipSubsystem의 OnQuipFired에 구독해 자동 표시.
 * 진행 중 신규 라인 도착 시 soft replace (현 라인 Exit → 신규 Enter).
 */
UCLASS()
class PROJECTD_API UPDQuipToastWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 외부에서 직접 멘트 표시. 일반 경로는 Subsystem 자동. */
	UFUNCTION(BlueprintCallable, Category = "PD|Quip")
	void ShowLine(const FText& Line, float DisplaySeconds);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Line;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UBorder> Border_BG;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> Anim_Enter;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> Anim_Exit;

private:
	enum class EQuipPhase : uint8
	{
		Idle,
		Entering,
		Holding,
		Exiting
	};

	void HandleQuipFired(const FPDQuipEntry& Entry, const FText& Line);
	void StartShow(const FText& Line, float DisplaySeconds);
	void StartExit();

	UFUNCTION() void OnEnterAnimFinishedHandler();
	UFUNCTION() void OnExitAnimFinishedHandler();

	EQuipPhase Phase = EQuipPhase::Idle;
	float CurrentDisplaySeconds = 0.f;

	FText PendingLine;
	float PendingDisplaySeconds = 0.f;
	bool bHasPending = false;

	FTimerHandle HoldTimerHandle;
	FDelegateHandle QuipFiredHandle;
	TWeakObjectPtr<UPDQuipSubsystem> CachedSubsystem;
};