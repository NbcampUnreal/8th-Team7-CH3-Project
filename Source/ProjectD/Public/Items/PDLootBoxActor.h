#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/PDInteractable.h"
#include "PDLootBoxActor.generated.h"

class APDPlayerController;
class UBoxComponent;
class UPDLootComponent;
class UStaticMeshComponent;
class USoundBase;

/**
 * 적 사망 시 스폰되는 일회용 LootBox.
 *  - Personal Stash 와 코드 무관 (별도 클래스 계층).
 *  - LootComponent 의 콘텐츠는 모든 플레이어 공유 (컴포넌트가 전체 리플리케이션).
 *  - 액터 자체도 리플리케이션 — 호스트가 스폰하면 클라에서도 보임.
 */
UCLASS(Blueprintable)
class PROJECTD_API APDLootBoxActor : public AActor, public IPDInteractable
{
	GENERATED_BODY()

public:
	APDLootBoxActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "PD|LootBox")
	FORCEINLINE UPDLootComponent* GetLootComponent() const { return LootComponent; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|LootBox")
	TObjectPtr<UBoxComponent> InteractionCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|LootBox")
	TObjectPtr<UStaticMeshComponent> BoxMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|LootBox")
	TObjectPtr<UPDLootComponent> LootComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Sound")
	TObjectPtr<USoundBase> OpenSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Sound")
	TObjectPtr<USoundBase> CloseSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Sound", meta = (ClampMin = "0.0"))
	float SoundVolumeMultiplier = 1.f;

	// ─── Highlight (CustomDepth Stencil) ─────────────────────────────────
	// PP 머티리얼이 CustomStencil 값을 알파로 사용. C++ 는 거리 기반으로 값만 갱신.

	/** 본 거리(cm) 안에서 강조 표시. 밖이면 CustomDepth 자동 끔(렌더 비용 0). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Highlight", meta = (ClampMin = "0.0"))
	float HighlightMaxDistance = 800.f;

	/** 본 거리(cm) 이하에서 최대 강도(255). MinDistance ≤ MaxDistance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Highlight", meta = (ClampMin = "0.0"))
	float HighlightMinDistance = 150.f;

	/** MaxDistance 경계에서의 최소 스텐실 값(1~255). 너무 낮으면 가장자리에서 안 보임. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Highlight", meta = (ClampMin = "1", ClampMax = "255"))
	int32 HighlightMinStencil = 32;

	/** Tick 갱신 주기(초). 0 이면 매 프레임. 0.05 정도로 throttle 가능 — 시각적 차이 미미. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Highlight", meta = (ClampMin = "0.0"))
	float HighlightUpdateInterval = 0.0f;

private:
	void ConfigureInteractionCollision() const;
	void PlayInteractSound(bool bOpen) const;
	void BindLootClose(APDPlayerController* PlayerController);
	void UnbindLootClose();

	// 로컬 플레이어와의 거리 기반으로 메시 CustomDepthStencilValue 갱신.
	void UpdateHighlightStencil();

	UFUNCTION()
	void HandleLootInterfaceClosed(UPDLootComponent* ClosedLootComponent);

	TWeakObjectPtr<APDPlayerController> BoundPlayerController;

	// Tick interval 정확히 지키기 위한 누적 시간. HighlightUpdateInterval > 0 일 때만 사용.
	float HighlightAccumulator = 0.f;
};
