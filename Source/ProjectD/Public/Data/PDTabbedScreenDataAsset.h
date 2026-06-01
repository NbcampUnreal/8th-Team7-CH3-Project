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

	/** 탭이 활성화될 때 풋터의 TXT_Description에 자동 채워진다. 비어있으면 풋터 텍스트 클리어. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Tabbed", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Tabbed", meta = (MustImplement = "/Script/ProjectD.PDTabbedContent"))
	TSoftClassPtr<UUserWidget> ContentClass;

	/**
	 * 활성 탭일 때 NamedSlot_MainLeft에 동적 부착되는 좌측 패널.
	 * nullptr이면 활성 시 NamedSlot이 명시적으로 비워짐(잔상 방지). DA가 컨텐츠를 결정.
	 * IPDTabbedContent를 구현하면 InitializeForOwner / OnEmbeddedInHub를 받음 (라이프사이클 콜백은 메인 컨텐츠 전용).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Tabbed|Panels")
	TSoftClassPtr<UUserWidget> LeftPanelClass;

	/** 좌측 패널과 동일 규칙. NamedSlot_MainRight에 부착. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Tabbed|Panels")
	TSoftClassPtr<UUserWidget> RightPanelClass;

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