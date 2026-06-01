#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PDActionPromptDataAsset.generated.h"

class UInputAction;

USTRUCT(BlueprintType)
struct FPDActionPromptEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> InputAction;

	// HUD에 표시될 액션 이름 (예: "인벤토리 열기")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText DisplayName;
};

// HUD 우하단에 노출할 상호작용 키 가이드 목록. 항목 추가/순서 변경은 이 DA에서만.
UCLASS(BlueprintType)
class PROJECTD_API UPDActionPromptDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ActionPrompt")
	TArray<FPDActionPromptEntry> Entries;
};