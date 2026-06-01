#pragma once

#include "CoreMinimal.h"
#include "GameplayCueManager.h"
#include "GameplayTagContainer.h"
#include "PDGameplayCueManager.generated.h"

class UGameplayCueNotify_Static;
class UGameplayCueSet;

UCLASS()
class PROJECTD_API UPDGameplayCueManager : public UGameplayCueManager
{
	GENERATED_BODY()

public:
	virtual void OnCreated() override;
	virtual void OnEngineInitComplete() override;
	virtual void RouteGameplayCue(AActor* TargetActor, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters, EGameplayCueExecutionOptions Options = EGameplayCueExecutionOptions::Default) override;
	virtual bool ShouldAsyncLoadObjectLibrariesAtStart() const override { return false; }
	virtual bool ShouldAsyncLoadRuntimeObjectLibraries() const override;
	virtual bool ShouldSyncLoadRuntimeObjectLibraries() const override;

private:
	void RegisterNativeGameplayCues();
	void RegisterNativeGameplayCue(UGameplayCueSet* CueSet, const FGameplayTag& CueTag, TSubclassOf<UGameplayCueNotify_Static> CueClass) const;
	TSubclassOf<UGameplayCueNotify_Static> GetNativeGameplayCueClass(const FGameplayTag& CueTag) const;
};
