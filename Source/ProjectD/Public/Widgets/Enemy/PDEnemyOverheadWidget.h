#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Enemy/Types/EnemyTypes.h"
#include "PDEnemyOverheadWidget.generated.h"

class UProgressBar;
class UImage;
class UWidget;

/**
 * 적 머리 위 HUD — Torso HP 바 + 상태 알림 아이콘.
 *  - HP 바: 초기 collapsed. ShowHealth() 호출 후 영구 표시. SetHealth(Current, Max) 로 갱신.
 *  - 상태 아이콘: Alert → 노란 느낌표(AlertIcon), Combat → 빨간 느낌표(CombatIcon). 그 외 상태는 둘 다 숨김.
 *  - ShowSpeech(NewState, Duration) 호출 시 Duration 초 후 자동 숨김.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDEnemyOverheadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Torso HP push. HealthRoot 가 collapsed 면 표시되지 않음 — ShowHealth() 선행 호출.
	UFUNCTION(BlueprintCallable, Category = "PD|Enemy|HUD")
	void SetHealth(float Current, float Max);

	// 첫 피격 시 HP 바 영구 노출.
	UFUNCTION(BlueprintCallable, Category = "PD|Enemy|HUD")
	void ShowHealth();

	// 상태 전환 알림 — Duration 초 후 자동 숨김. 0 이하면 수동 호출까지 유지.
	UFUNCTION(BlueprintCallable, Category = "PD|Enemy|HUD")
	void ShowSpeech(EPDEnemyState NewState, float Duration);

	// 사망 등 강제 일괄 숨김 — 타이머도 정리.
	UFUNCTION(BlueprintCallable, Category = "PD|Enemy|HUD")
	void HideAll();

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	// HP 표시 토글 대상 컨테이너. BP 트리에서 동명 이름의 패널을 두면 자동 바인딩.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> HealthRoot;

	// Alert 상태 — 노란 느낌표. BP 측에서 같은 이름의 UImage 배치.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> AlertIcon;

	// Combat 상태 — 빨간 느낌표. BP 측에서 같은 이름의 UImage 배치.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> CombatIcon;

private:
	UFUNCTION()
	void HandleSpeechTimeout();

	FTimerHandle SpeechTimerHandle;

	// 가장 최근에 ShowSpeech 가 처리한 state. 같은 state 가 노출 유효기간 안에 다시 들어오면 무시(깜빡임 방지).
	// 타이머 만료/HideAll 시 Idle 로 리셋 → 자연스럽게 재표시 허용.
	EPDEnemyState LastShownState = EPDEnemyState::Idle;
};
