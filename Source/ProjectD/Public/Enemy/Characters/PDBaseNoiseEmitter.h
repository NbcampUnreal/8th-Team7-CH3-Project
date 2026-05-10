#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDBaseNoiseEmitter.generated.h"

/**
 * 디자이너/QA 용 청각 자극 발생기.
 * EmitNoise 호출 시 UAISense_Hearing::ReportNoiseEvent 로 주변 AI 의 PerceptionComponent 에 전달.
 * 트리거 박스 / 타이머 / 시퀀서 이벤트 등에서 호출하여 AI 반응 디버깅 가능.
 */
UCLASS(Blueprintable)
class PROJECTD_API APDBaseNoiseEmitter : public AActor
{
	GENERATED_BODY()

public:
	APDBaseNoiseEmitter();

	UFUNCTION(BlueprintCallable, Category = "PD|Noise")
	void EmitNoise();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Noise", meta = (ClampMin = "0.0"))
	float MaxNoiseRange = 1500.f;

	/** ±RandomDeviation 로 Range 흔들기. AI 가 정확한 거리 측정으로 학습되지 않도록. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Noise", meta = (ClampMin = "0.0"))
	float NoiseRangeRandomDeviation = 100.f;

	/** Hearing 자극의 태그 — 디자이너가 자극 종류를 분류할 때 사용. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Noise")
	FName NoiseTag = TEXT("Noise");
};
