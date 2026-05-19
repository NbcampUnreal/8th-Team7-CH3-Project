#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Type/Types.h"
#include "PDItemGradeColorData.generated.h"

/**
 * 아이템 등급 → UI 표시 색상 매핑.
 * 인벤토리/스태시 슬롯 배경 등 등급 강조가 필요한 모든 위젯에서 공통 참조.
 * 디자이너가 PIE 없이 DA에서 직접 튜닝.
 */
UCLASS(BlueprintType)
class PROJECTD_API UPDItemGradeColorData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|ItemGrade")
	TMap<EPDItemGrade, FLinearColor> GradeColors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|ItemGrade")
	FLinearColor FallbackColor = FLinearColor::White;

	UFUNCTION(BlueprintCallable, Category = "PD|ItemGrade")
	FLinearColor ResolveColor(EPDItemGrade Grade) const;
};