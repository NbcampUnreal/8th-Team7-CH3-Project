#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PDTabbedScreenDataAsset.generated.h"

class UUserWidget;
class UTexture2D;

/**
 * 탭 한 개의 정의. ContentClass는 IPDTabbedContent를 반드시 구현해야 한다.
 */
USTRUCT(BlueprintType)
struct FPDTabbedScreenEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Tabbed")
	FGameplayTag TabId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Tabbed")
	FText Label;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Tabbed", meta = (MustImplement = "/Script/ProjectD.PDTabbedContent"))
	TSoftClassPtr<UUserWidget> ContentClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Tabbed")
	TSoftObjectPtr<UTexture2D> Icon;

	/** false면 탭 버튼을 회색 처리하고 클릭을 무시한다(스킬 탭 placeholder 등). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Tabbed")
	bool bEnabled = true;
};

/**
 * 탭 화면(UPDTabbedScreenBase 파생)의 탭 집합. Hub/Options 등 화면별로 인스턴스를 만들어 할당한다.
 */
UCLASS(BlueprintType)
class PROJECTD_API UPDTabbedScreenDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Tabbed")
	TArray<FPDTabbedScreenEntry> Tabs;

	/** 비어있으면 첫 활성 탭이 기본값. bRememberLastTab=true면 이전 선택을 우선. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Tabbed")
	FGameplayTag DefaultTabId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Tabbed")
	bool bRememberLastTab = true;
};