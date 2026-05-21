#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/PDInteractable.h"
#include "PDLootBoxActor.generated.h"

class APDPlayerController;
class UBoxComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
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

	// ─── Highlight (Overlay Material) ────────────────────────────────────
	// BoxMesh 의 Overlay Material 슬롯에 MID 부착, 거리 기반으로 OLIntensity/OLThickness 보간.

	/** Overlay Material 자산(MM_Outline 등). MID 로 동적 파라미터 제어. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Highlight")
	TObjectPtr<UMaterialInterface> OutlineMaterial;

	/** 본 거리(cm) 안에서만 외곽선 표시. 밖이면 Overlay Material 해제(드로콜 0). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Highlight", meta = (ClampMin = "0.0"))
	float HighlightMaxDistance = 800.f;

	/** 본 거리(cm) 이하에서 최대 강도. MinDistance ≤ MaxDistance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Highlight", meta = (ClampMin = "0.0"))
	float HighlightMinDistance = 0.f;

	/** Falloff 곡선 지수. 1=선형, 2~3=가까이서 급격히 강해짐(권장). Intensity/Thickness 둘 다 적용. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Highlight", meta = (ClampMin = "0.1", ClampMax = "8.0"))
	float HighlightFalloffExponent = 2.5f;

	/** 경계 거리에서의 외곽선 강도. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Highlight", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OutlineIntensityMin = 0.1f;

	/** 최소 거리(=가까움)에서의 외곽선 강도. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Highlight", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float OutlineIntensityMax = 100.0f;

	/** 경계 거리에서의 외곽선 두께. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Highlight", meta = (ClampMin = "0.0"))
	float OutlineThicknessMin = 0.0f;

	/** 최소 거리(=가까움)에서의 외곽선 두께. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Highlight", meta = (ClampMin = "0.0"))
	float OutlineThicknessMax = 3.0f;

	/** MM_Outline 측 스칼라 파라미터 이름. 머티리얼과 스펠링/대소문자 일치 필수. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Highlight")
	FName IntensityParamName = TEXT("OLIntensity");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Highlight")
	FName ThicknessParamName = TEXT("OLThickness");

	/** Tick 갱신 주기(초). 0 이면 매 프레임. 0.05 정도로 throttle 가능 — 시각적 차이 미미. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|LootBox|Highlight", meta = (ClampMin = "0.0"))
	float HighlightUpdateInterval = 0.0f;

private:
	void ConfigureInteractionCollision() const;
	void PlayInteractSound(bool bOpen) const;
	void BindLootClose(APDPlayerController* PlayerController);
	void UnbindLootClose();

	// 로컬 플레이어와의 거리 기반으로 Overlay MID 파라미터 갱신.
	void UpdateOutlineParams();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OutlineMID;

	UFUNCTION()
	void HandleLootInterfaceClosed(UPDLootComponent* ClosedLootComponent);

	TWeakObjectPtr<APDPlayerController> BoundPlayerController;

	// Tick interval 정확히 지키기 위한 누적 시간. HighlightUpdateInterval > 0 일 때만 사용.
	float HighlightAccumulator = 0.f;
};
