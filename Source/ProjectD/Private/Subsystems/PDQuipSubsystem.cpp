#include "Subsystems/PDQuipSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

UPDQuipSubsystem* UPDQuipSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}
	UGameInstance* GI = World->GetGameInstance();
	return GI ? GI->GetSubsystem<UPDQuipSubsystem>() : nullptr;
}

void UPDQuipSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPDQuipSubsystem::Deinitialize()
{
	UnbindFromASC();
	EntryIndexByTag.Reset();
	LastFiredTime.Reset();
	LastPickedIndex.Reset();
	QuipDataAsset = nullptr;
	Super::Deinitialize();
}

void UPDQuipSubsystem::SetQuipDataAsset(UPDQuipDataAsset* InData)
{
	QuipDataAsset = InData;
	RebuildLookup();

	// DA 변경되면 신규 trigger 집합으로 ASC 재구독
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		RebindToASC(ASC);
	}
}

void UPDQuipSubsystem::RebuildLookup()
{
	EntryIndexByTag.Reset();
	if (!QuipDataAsset)
	{
		return;
	}

	for (int32 i = 0; i < QuipDataAsset->Entries.Num(); ++i)
	{
		const FPDQuipEntry& Entry = QuipDataAsset->Entries[i];
		if (!Entry.TriggerTag.IsValid() || Entry.Lines.Num() == 0)
		{
			continue;
		}
		// 동일 TriggerTag 중복 시 후방 entry로 덮어씀 (디자이너 실수 보호용 ensure 가능하지만 일단 silent)
		EntryIndexByTag.Add(Entry.TriggerTag, i);
	}
}

void UPDQuipSubsystem::NotifyPawnChanged(APawn* NewPawn)
{
	// IAbilitySystemInterface/PlayerState 둘 다 대응. PC HUD가 쓰는 카논 경로와 동일.
	UAbilitySystemComponent* NewASC = NewPawn
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(NewPawn)
		: nullptr;
	RebindToASC(NewASC);
}

void UPDQuipSubsystem::RebindToASC(UAbilitySystemComponent* NewASC)
{
	UnbindFromASC();
	if (!NewASC)
	{
		return;
	}

	CachedASC = NewASC;

	// DA의 모든 TriggerTag를 ASC에 등록. Event.* 같은 비-ASC 태그가 섞여도 발화하지 않으니 무해.
	// State.* 필터링을 RequestGameplayTag("State")로 시도하지 않는 이유: 부모 태그 미등록 시 invalid 반환 → 전체 스킵 위험.
	for (const TPair<FGameplayTag, int32>& Pair : EntryIndexByTag)
	{
		const FGameplayTag& Tag = Pair.Key;

		FDelegateHandle Handle = NewASC
			->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UPDQuipSubsystem::HandleAscTagChanged);

		SubscribedTags.Add(Tag);
		AscTagHandles.Add(Handle);
	}
}

void UPDQuipSubsystem::UnbindFromASC()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		const int32 Count = FMath::Min(SubscribedTags.Num(), AscTagHandles.Num());
		for (int32 i = 0; i < Count; ++i)
		{
			ASC->RegisterGameplayTagEvent(SubscribedTags[i], EGameplayTagEventType::NewOrRemoved)
				.Remove(AscTagHandles[i]);
		}
	}
	SubscribedTags.Reset();
	AscTagHandles.Reset();
	CachedASC.Reset();
}

void UPDQuipSubsystem::HandleAscTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		RequestQuipByTag(Tag);
	}
}

void UPDQuipSubsystem::RequestQuipByTag(FGameplayTag TriggerTag)
{
	if (!TriggerTag.IsValid() || !QuipDataAsset)
	{
		return;
	}

	const int32* IdxPtr = EntryIndexByTag.Find(TriggerTag);
	if (!IdxPtr)
	{
		return;
	}

	const FPDQuipEntry& Entry = QuipDataAsset->Entries[*IdxPtr];
	if (Entry.Lines.Num() == 0)
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();
	if (const double* LastPtr = LastFiredTime.Find(TriggerTag))
	{
		if ((Now - *LastPtr) < Entry.CooldownSeconds)
		{
			return;
		}
	}

	int32 Pick = FMath::RandRange(0, Entry.Lines.Num() - 1);
	if (Entry.Lines.Num() > 1)
	{
		if (const int32* LastIdxPtr = LastPickedIndex.Find(TriggerTag))
		{
			if (Pick == *LastIdxPtr)
			{
				Pick = (Pick + 1) % Entry.Lines.Num();
			}
		}
	}

	LastFiredTime.Add(TriggerTag, Now);
	LastPickedIndex.Add(TriggerTag, Pick);

	OnQuipFired.Broadcast(Entry, Entry.Lines[Pick]);
}