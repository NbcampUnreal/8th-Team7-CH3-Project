#include "GAS/Cues/PDGameplayCueManager.h"

#include "GAS/Cues/PDWeaponGameplayCues.h"
#include "GameplayCueSet.h"
#include "GameplayCueNotify_Static.h"
#include "GameplayTag/PDGameplayTags.h"

void UPDGameplayCueManager::OnCreated()
{
	Super::OnCreated();
	InitializeRuntimeObjectLibrary();
	RegisterNativeGameplayCues();
}

void UPDGameplayCueManager::OnEngineInitComplete()
{
	Super::OnEngineInitComplete();
	RegisterNativeGameplayCues();
}

bool UPDGameplayCueManager::ShouldAsyncLoadRuntimeObjectLibraries() const
{
	return false;
}

bool UPDGameplayCueManager::ShouldSyncLoadRuntimeObjectLibraries() const
{
	return true;
}

void UPDGameplayCueManager::RouteGameplayCue(AActor* TargetActor, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters, EGameplayCueExecutionOptions Options)
{
	if (TargetActor && !(Options & EGameplayCueExecutionOptions::IgnoreNotifies))
	{
		if (TSubclassOf<UGameplayCueNotify_Static> CueClass = GetNativeGameplayCueClass(GameplayCueTag))
		{
			if (UGameplayCueNotify_Static* CueNotify = Cast<UGameplayCueNotify_Static>(CueClass->GetDefaultObject(false)))
			{
				if (CueNotify->HandlesEvent(EventType))
				{
					CueNotify->HandleGameplayCue(TargetActor, EventType, Parameters);
					TargetActor->ForceNetUpdate();
					return;
				}
			}
		}
	}

	Super::RouteGameplayCue(TargetActor, GameplayCueTag, EventType, Parameters, Options);
}

void UPDGameplayCueManager::RegisterNativeGameplayCues()
{
	UGameplayCueSet* CueSet = GetRuntimeCueSet();
	if (!CueSet)
	{
		return;
	}

	RegisterNativeGameplayCue(CueSet, PDGameplayTags::GameplayCue_Weapon_Fire, UGCN_Weapon_Fire::StaticClass());
	RegisterNativeGameplayCue(CueSet, PDGameplayTags::GameplayCue_Weapon_Impact, UGCN_Weapon_Impact::StaticClass());
	RegisterNativeGameplayCue(CueSet, PDGameplayTags::GameplayCue_Weapon_Equip, UGCN_Weapon_Equip::StaticClass());
	RegisterNativeGameplayCue(CueSet, PDGameplayTags::GameplayCue_Weapon_Swing, UGCN_Weapon_Swing::StaticClass());
	RegisterNativeGameplayCue(CueSet, PDGameplayTags::GameplayCue_Weapon_MeleeHit, UGCN_Weapon_MeleeHit::StaticClass());
	RegisterNativeGameplayCue(CueSet, PDGameplayTags::GameplayCue_Weapon_CartridgeHit, UGCN_Weapon_CartridgeHit::StaticClass());
	RegisterNativeGameplayCue(CueSet, PDGameplayTags::GameplayCue_Weapon_Reload, UGCN_Weapon_Reload::StaticClass());
	RegisterNativeGameplayCue(CueSet, PDGameplayTags::GameplayCue_Weapon_ReloadEmpty, UGCN_Weapon_ReloadEmpty::StaticClass());

	RegisterNativeGameplayCue(CueSet, PDGameplayTags::GameplayCue_Character_HitReact, UGCN_Character_HitReact::StaticClass());
	RegisterNativeGameplayCue(CueSet, PDGameplayTags::GameplayCue_Character_Bleeding, UGCN_Character_Bleeding::StaticClass());
	RegisterNativeGameplayCue(CueSet, PDGameplayTags::GameplayCue_Character_Roll, UGCN_Character_Roll::StaticClass());
	RegisterNativeGameplayCue(CueSet, PDGameplayTags::GameplayCue_Character_Footstep, UGCN_Character_Footstep::StaticClass());

	RegisterNativeGameplayCue(CueSet, PDGameplayTags::GameplayCue_Item_Pickup, UGCN_Item_Pickup::StaticClass());
}

void UPDGameplayCueManager::RegisterNativeGameplayCue(UGameplayCueSet* CueSet, const FGameplayTag& CueTag, TSubclassOf<UGameplayCueNotify_Static> CueClass) const
{
	if (!CueSet || !CueTag.IsValid() || !CueClass)
	{
		return;
	}

	const int32 ExistingIndex = CueSet->GameplayCueData.IndexOfByPredicate(
		[&CueTag](const FGameplayCueNotifyData& Data)
		{
			return Data.GameplayCueTag == CueTag;
		});

	const int32 CueDataIndex = ExistingIndex != INDEX_NONE ? ExistingIndex : CueSet->GameplayCueData.AddDefaulted();
	FGameplayCueNotifyData& CueData = CueSet->GameplayCueData[CueDataIndex];
	CueData.GameplayCueTag = CueTag;
	CueData.GameplayCueNotifyObj = FSoftObjectPath(CueClass.Get());
	CueData.LoadedGameplayCueClass = CueClass.Get();
	CueData.ParentDataIdx = INDEX_NONE;

	CueSet->GameplayCueDataMap.Add(CueTag, CueDataIndex);
}

TSubclassOf<UGameplayCueNotify_Static> UPDGameplayCueManager::GetNativeGameplayCueClass(const FGameplayTag& CueTag) const
{
	if (CueTag == PDGameplayTags::GameplayCue_Weapon_Fire) return UGCN_Weapon_Fire::StaticClass();
	if (CueTag == PDGameplayTags::GameplayCue_Weapon_Impact) return UGCN_Weapon_Impact::StaticClass();
	if (CueTag == PDGameplayTags::GameplayCue_Weapon_Equip) return UGCN_Weapon_Equip::StaticClass();
	if (CueTag == PDGameplayTags::GameplayCue_Weapon_Swing) return UGCN_Weapon_Swing::StaticClass();
	if (CueTag == PDGameplayTags::GameplayCue_Weapon_MeleeHit) return UGCN_Weapon_MeleeHit::StaticClass();
	if (CueTag == PDGameplayTags::GameplayCue_Weapon_CartridgeHit) return UGCN_Weapon_CartridgeHit::StaticClass();
	if (CueTag == PDGameplayTags::GameplayCue_Weapon_Reload) return UGCN_Weapon_Reload::StaticClass();
	if (CueTag == PDGameplayTags::GameplayCue_Weapon_ReloadEmpty) return UGCN_Weapon_ReloadEmpty::StaticClass();

	if (CueTag == PDGameplayTags::GameplayCue_Character_HitReact) return UGCN_Character_HitReact::StaticClass();
	if (CueTag == PDGameplayTags::GameplayCue_Character_Bleeding) return UGCN_Character_Bleeding::StaticClass();
	if (CueTag == PDGameplayTags::GameplayCue_Character_Roll) return UGCN_Character_Roll::StaticClass();
	if (CueTag == PDGameplayTags::GameplayCue_Character_Footstep) return UGCN_Character_Footstep::StaticClass();

	if (CueTag == PDGameplayTags::GameplayCue_Item_Pickup) return UGCN_Item_Pickup::StaticClass();

	return nullptr;
}
