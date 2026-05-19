#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Data/PDQuipDataAsset.h"
#include "PDQuipSubsystem.generated.h"

class UAbilitySystemComponent;
class APawn;

DECLARE_MULTICAST_DELEGATE_TwoParams(FPDOnQuipFired, const FPDQuipEntry& /*Entry*/, const FText& /*Line*/);

/**
 * 캐릭터 Quip(멘트) 중앙 라우터.
 * DA(UPDQuipDataAsset)에 정의된 TriggerTag → Line 매핑을 따라 위젯에 Broadcast.
 *
 * 두 가지 트리거 경로:
 *   1) 명시 호출: 게임플레이 코드가 RequestQuipByTag(Tag) 호출
 *   2) ASC 태그: State.* 하위 트리거는 자동 구독해서 count>0 진입 시 발화
 */
UCLASS()
class PROJECTD_API UPDQuipSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UPDQuipSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** DA 주입. 호출 시점에 lookup map 재구축, 기존 ASC 구독도 갱신. */
	UFUNCTION(BlueprintCallable, Category = "PD|Quip")
	void SetQuipDataAsset(UPDQuipDataAsset* InData);

	/** 명시 트리거. State.* 외 Event.* 등은 이 경로로. */
	UFUNCTION(BlueprintCallable, Category = "PD|Quip")
	void RequestQuipByTag(FGameplayTag TriggerTag);

	/** PC가 Pawn possession 변경 시 호출. nullptr 허용(언바인드). */
	void NotifyPawnChanged(APawn* NewPawn);

	/** 위젯이 구독. (Entry, 선택된 Line) */
	FPDOnQuipFired OnQuipFired;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPDQuipDataAsset> QuipDataAsset;

	TMap<FGameplayTag, int32> EntryIndexByTag;
	TMap<FGameplayTag, double> LastFiredTime;
	TMap<FGameplayTag, int32> LastPickedIndex;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	TArray<FGameplayTag> SubscribedTags;
	TArray<FDelegateHandle> AscTagHandles;

	void RebuildLookup();
	void RebindToASC(UAbilitySystemComponent* NewASC);
	void UnbindFromASC();
	void HandleAscTagChanged(const FGameplayTag Tag, int32 NewCount);
};