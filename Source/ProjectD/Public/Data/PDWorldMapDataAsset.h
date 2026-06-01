#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PDWorldMapDataAsset.generated.h"

class UTexture2D;
class UWorld;

//한 레벨의 월드맵 정보
USTRUCT(BlueprintType)
struct FPDWorldMapEntry
{
	GENERATED_BODY()

	//맵 배경 텍스처 (캡처해서 만든 거)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap")
	TObjectPtr<UTexture2D> MapTexture;

	//캡처 카메라의 (X, Y) 월드 좌표
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap")
	FVector2D WorldCenter = FVector2D::ZeroVector;

	//캡처 카메라의 Ortho Width(cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap", meta=(ClampMin="100.0"))
	float WorldSize = 10000.f;

	//플레이어 화살표 회전 보정값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap")
	float PlayerArrowAngleOffset = 0.f;
};

//레벨별 월드맵 매핑. 레벨 추가 시 LevelMaps에 entry만 추가하면 됨
UCLASS(BlueprintType)
class PROJECTD_API UPDWorldMapDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	//레벨 에셋 => 맵 정보 매핑
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap")
	TMap<TSoftObjectPtr<UWorld>, FPDWorldMapEntry> LevelMaps;

	//매핑 없는 레벨 fallback
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WorldMap")
	FPDWorldMapEntry DefaultEntry;

	//현재 월드에 맞는 entry 가져옴. 없으면 DefaultEntry
	UFUNCTION(BlueprintCallable, Category="WorldMap")
	FPDWorldMapEntry GetEntryForWorld(const UWorld* InWorld) const;
};