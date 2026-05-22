#include "Widgets/Screen/PDTabbedScreenBase.h"

#include "Widgets/Screen/PDTabButtonWidget.h"
#include "Widgets/Screen/PDTabbedContent.h"
#include "Data/PDTabbedScreenDataAsset.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Components/HorizontalBox.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"
#include "Blueprint/UserWidget.h"

UPDTabbedScreenBase* UPDTabbedScreenBase::OpenAtTab(
	UObject* WorldContextObject,
	TSubclassOf<UPDTabbedScreenBase> ScreenClass,
	FGameplayTag TabId,
	EUILayer Layer)
{
	if (!ScreenClass)
	{
		return nullptr;
	}

	UPDFrontendUISubsystem* Subsystem = UPDFrontendUISubsystem::Get(WorldContextObject);
	if (!Subsystem)
	{
		return nullptr;
	}

	UPDActivatableBase* Pushed = Subsystem->PushToLayer(Layer, ScreenClass);
	UPDTabbedScreenBase* Tabbed = Cast<UPDTabbedScreenBase>(Pushed);
	if (Tabbed && TabId.IsValid())
	{
		Tabbed->SetInitialTab(TabId);
	}
	return Tabbed;
}

void UPDTabbedScreenBase::SetInitialTab(FGameplayTag TabId)
{
	PendingInitialTab = TabId;
	if (IsActivated() && TabId.IsValid())
	{
		SwitchToTab(TabId);
	}
}

void UPDTabbedScreenBase::SwitchToTab(FGameplayTag TabId)
{
	if (!TabId.IsValid() || !Switcher_Content)
	{
		return;
	}

	const int32 TargetIndex = FindTabIndexInDataAsset(TabId);
	if (TargetIndex == INDEX_NONE)
	{
		return;
	}

	if (TabSet && TabSet->Tabs.IsValidIndex(TargetIndex) && !TabSet->Tabs[TargetIndex].bEnabled)
	{
		return;
	}

	if (ActiveTabId.IsValid() && ActiveTabId != TabId)
	{
		if (TObjectPtr<UUserWidget>* PrevContent = SpawnedContents.Find(ActiveTabId))
		{
			if (*PrevContent && (*PrevContent)->GetClass()->ImplementsInterface(UPDTabbedContent::StaticClass()))
			{
				if (IPDTabbedContent* PrevIface = Cast<IPDTabbedContent>(*PrevContent))
				{
					PrevIface->OnTabHidden();
				}
			}
		}
	}

	ActiveTabId = TabId;
	LastTabId = TabId;

	if (TObjectPtr<UUserWidget>* NewContent = SpawnedContents.Find(TabId))
	{
		if (*NewContent)
		{
			Switcher_Content->SetActiveWidget(*NewContent);

			if ((*NewContent)->GetClass()->ImplementsInterface(UPDTabbedContent::StaticClass()))
			{
				if (IPDTabbedContent* NewIface = Cast<IPDTabbedContent>(*NewContent))
				{
					NewIface->OnTabShown();
					if (UWidget* Focus = NewIface->GetTabDesiredFocusTarget())
					{
						Focus->SetUserFocus(GetOwningPlayer());
					}
				}
			}
		}
	}

	ApplyActiveTabVisualState();
}

void UPDTabbedScreenBase::CycleTab(int32 Direction)
{
	if (!TabSet || TabSet->Tabs.Num() == 0 || Direction == 0)
	{
		return;
	}

	const int32 N = TabSet->Tabs.Num();
	int32 CurrentIdx = FindTabIndexInDataAsset(ActiveTabId);
	if (CurrentIdx == INDEX_NONE)
	{
		CurrentIdx = 0;
	}

	const int32 NormalizedDir = Direction > 0 ? 1 : -1;
	for (int32 Step = 1; Step <= N; ++Step)
	{
		const int32 NextIdx = ((CurrentIdx + NormalizedDir * Step) % N + N) % N;
		const FPDTabbedScreenEntry& Entry = TabSet->Tabs[NextIdx];
		if (Entry.bEnabled && Entry.TabId.IsValid() && Entry.TabId != ActiveTabId)
		{
			SwitchToTab(Entry.TabId);
			return;
		}
	}
}

void UPDTabbedScreenBase::NativeOnActivated()
{
	Super::NativeOnActivated();

	RebuildTabsAndContent();

	const FGameplayTag InitialId = ResolveInitialTabId();
	if (InitialId.IsValid())
	{
		SwitchToTab(InitialId);
	}

	PendingInitialTab = FGameplayTag();
}

void UPDTabbedScreenBase::NativeOnDeactivated()
{
	if (ActiveTabId.IsValid())
	{
		if (TObjectPtr<UUserWidget>* ActiveContent = SpawnedContents.Find(ActiveTabId))
		{
			if (*ActiveContent && (*ActiveContent)->GetClass()->ImplementsInterface(UPDTabbedContent::StaticClass()))
			{
				if (IPDTabbedContent* Iface = Cast<IPDTabbedContent>(*ActiveContent))
				{
					Iface->OnTabHidden();
				}
			}
		}
	}

	ClearTabsAndContent();
	ActiveTabId = FGameplayTag();
	Super::NativeOnDeactivated();
}

void UPDTabbedScreenBase::HandleTabButtonClicked(FGameplayTag ClickedTabId)
{
	SwitchToTab(ClickedTabId);
}

void UPDTabbedScreenBase::RebuildTabsAndContent()
{
	ClearTabsAndContent();

	if (!TabSet || !HBox_TabBar || !Switcher_Content)
	{
		return;
	}

	APlayerController* OwnerPC = GetOwningPlayer();

	for (const FPDTabbedScreenEntry& Entry : TabSet->Tabs)
	{
		if (!Entry.TabId.IsValid())
		{
			continue;
		}

		if (TabButtonClass)
		{
			UPDTabButtonWidget* TabButton = CreateWidget<UPDTabButtonWidget>(OwnerPC, TabButtonClass);
			if (TabButton)
			{
				UTexture2D* IconTex = Entry.Icon.IsNull() ? nullptr : Entry.Icon.LoadSynchronous();
				TabButton->InitializeTabButton(Entry.TabId, Entry.Label, IconTex, Entry.bEnabled);
				TabButton->OnTabButtonClicked.AddDynamic(this, &UPDTabbedScreenBase::HandleTabButtonClicked);
				HBox_TabBar->AddChild(TabButton);
				SpawnedButtons.Add(Entry.TabId, TabButton);
			}
		}

		UClass* ContentClassLoaded = Entry.ContentClass.IsNull() ? nullptr : Entry.ContentClass.LoadSynchronous();
		if (!ContentClassLoaded)
		{
			continue;
		}

		UUserWidget* ContentWidget = CreateWidget<UUserWidget>(OwnerPC, ContentClassLoaded);
		if (!ContentWidget)
		{
			continue;
		}

		Switcher_Content->AddChild(ContentWidget);
		SpawnedContents.Add(Entry.TabId, ContentWidget);

		if (ContentWidget->GetClass()->ImplementsInterface(UPDTabbedContent::StaticClass()))
		{
			if (IPDTabbedContent* Iface = Cast<IPDTabbedContent>(ContentWidget))
			{
				Iface->InitializeForOwner(OwnerPC);
			}
		}
	}
}

void UPDTabbedScreenBase::ClearTabsAndContent()
{
	SpawnedContents.Reset();
	SpawnedButtons.Reset();
	if (HBox_TabBar)
	{
		HBox_TabBar->ClearChildren();
	}
	if (Switcher_Content)
	{
		Switcher_Content->ClearChildren();
	}
}

int32 UPDTabbedScreenBase::FindTabIndexInDataAsset(FGameplayTag TabId) const
{
	if (!TabSet)
	{
		return INDEX_NONE;
	}
	for (int32 Index = 0; Index < TabSet->Tabs.Num(); ++Index)
	{
		if (TabSet->Tabs[Index].TabId == TabId)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

void UPDTabbedScreenBase::ApplyActiveTabVisualState()
{
	for (const TPair<FGameplayTag, TObjectPtr<UPDTabButtonWidget>>& Pair : SpawnedButtons)
	{
		if (Pair.Value)
		{
			Pair.Value->SetSelected(Pair.Key == ActiveTabId);
		}
	}
}

FGameplayTag UPDTabbedScreenBase::ResolveInitialTabId() const
{
	if (PendingInitialTab.IsValid())
	{
		return PendingInitialTab;
	}
	if (TabSet && TabSet->bRememberLastTab && LastTabId.IsValid())
	{
		return LastTabId;
	}
	if (TabSet && TabSet->DefaultTabId.IsValid())
	{
		return TabSet->DefaultTabId;
	}
	if (TabSet)
	{
		for (const FPDTabbedScreenEntry& Entry : TabSet->Tabs)
		{
			if (Entry.bEnabled && Entry.TabId.IsValid())
			{
				return Entry.TabId;
			}
		}
	}
	return FGameplayTag();
}