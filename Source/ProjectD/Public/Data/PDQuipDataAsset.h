#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PDQuipDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FPDQuipEntry
{
	GENERATED_BODY()

	// 이 멘트를 유발하는 태그. 기존 State.* 직접 매칭 또는 신규 Event.Quip.* 사용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "State,Event"))
	FGameplayTag TriggerTag;

	// 같은 트리거의 후보 라인 (랜덤 픽으로 반복 피로 방지). 비어있으면 entry 무시.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FText> Lines;

	// 표시 시간 (Hold 단계). 기본 1.7s.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "0.1"))
	float DisplaySeconds = 1.7f;

	// 같은 TriggerTag 재발화 억제. 0이면 즉시 재발화 허용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "0.0"))
	float CooldownSeconds = 3.0f;
};

/**
 * 캐릭터 Quip(멘트) 데이터.
 * TriggerTag → Lines 매핑을 디자이너가 PIE 없이 튜닝.
 * UPDQuipSubsystem이 시작 시 로드해 태그 라우팅에 사용.
 */
UCLASS(BlueprintType)
class PROJECTD_API UPDQuipDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Quip")
	TArray<FPDQuipEntry> Entries;
};