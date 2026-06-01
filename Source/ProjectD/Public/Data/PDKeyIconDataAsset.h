#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputCoreTypes.h"
#include "PDKeyIconDataAsset.generated.h"

class UTexture2D;

// FKey → 키캡 이미지 매핑. 리바인드돼도 매핑만 잡혀있으면 자동 반영
UCLASS(BlueprintType)
class PROJECTD_API UPDKeyIconDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KeyIcon")
	TMap<FKey, TSoftObjectPtr<UTexture2D>> KeyIconMap;

	// 매핑 없는 키 fallback (없으면 nullptr 반환 → 위젯에서 Hidden 처리)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KeyIcon")
	TSoftObjectPtr<UTexture2D> FallbackIcon;

	UFUNCTION(BlueprintCallable, Category="KeyIcon")
	UTexture2D* ResolveIcon(const FKey& Key) const;
};