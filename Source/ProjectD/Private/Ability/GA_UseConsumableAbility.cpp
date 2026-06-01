#include "Ability/GA_UseConsumableAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Characters/PDPlayerCharacter.h"
#include "Core/PDPlayerComponentResolver.h"
#include "GameplayEffect.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Containers/PDQuickSlotComponent.h"

UGA_UseConsumableAbility::UGA_UseConsumableAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	TriggerData.TriggerTag = PDGameplayTags::Event_Item_UseConsumable;
	AbilityTriggers.Add(TriggerData);

	ActivationOwnedTags.AddTag(PDGameplayTags::State_UsingConsumable);
	ActivationBlockedTags.AddTag(PDGameplayTags::State_UsingConsumable);
	ActivationBlockedTags.AddTag(PDGameplayTags::State_Rolling);
	ActivationBlockedTags.AddTag(PDGameplayTags::State_Downed);
	ActivationBlockedTags.AddTag(PDGameplayTags::State_GettingUp);
	ActivationBlockedTags.AddTag(PDGameplayTags::State_Dead);
}

void UGA_UseConsumableAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ActiveRequest = TriggerEventData
		? const_cast<UPDConsumableUseRequest*>(Cast<UPDConsumableUseRequest>(TriggerEventData->OptionalObject.Get()))
		: nullptr;

	if (!ActiveRequest || ActiveRequest->ItemSlot.IsEmpty() ||
		ActiveRequest->ItemSlot.ItemData.ItemType != EPDItemType::Consumable)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UPDInventoryComponent* InventoryComponent = ResolveInventoryComponent();
	if (!InventoryComponent || !InventoryComponent->HasItem(ActiveRequest->ItemSlot.ItemData.ItemID, 1))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UseMoveSpeedEffectClass)
	{
		MoveSpeedEffectHandle = ApplyGameplayEffectToOwner(
			Handle,
			ActorInfo,
			ActivationInfo,
			UseMoveSpeedEffectClass.GetDefaultObject(),
			GetAbilityLevel(Handle, ActorInfo));
	}

	UAbilityTask_WaitGameplayEvent* CancelTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		PDGameplayTags::Event_Item_CancelConsumableUse,
		nullptr,
		false,
		true);
	if (CancelTask)
	{
		CancelTask->EventReceived.AddDynamic(this, &UGA_UseConsumableAbility::OnCancelConsumableUse);
		CancelTask->ReadyForActivation();
	}

	const float UseDuration = FMath::Max(0.f, ActiveRequest->ItemSlot.ItemData.UseDuration);
	if (UseDuration <= 0.f)
	{
		FinishConsumableUse();
		return;
	}

	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, UseDuration);
	if (!DelayTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	DelayTask->OnFinish.AddDynamic(this, &UGA_UseConsumableAbility::FinishConsumableUse);
	DelayTask->ReadyForActivation();
}

void UGA_UseConsumableAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	RemoveMoveSpeedEffect();

	if (bWasCancelled && ActiveRequest && ActiveRequest->QuickSlotComponent)
	{
		ActiveRequest->QuickSlotComponent->NotifyConsumableAbilityCanceled(
			ActiveRequest->QuickSlotIndex,
			ActiveRequest->ItemSlot.ItemData);
	}

	ActiveRequest = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_UseConsumableAbility::FinishConsumableUse()
{
	if (!ActiveRequest)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	UPDInventoryComponent* InventoryComponent = ResolveInventoryComponent();
	if (!InventoryComponent || !RemoveConsumableFromInventory(InventoryComponent, ActiveRequest->ItemSlot))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	ApplyConsumableEffect(ActiveRequest->ItemSlot);

	if (ActiveRequest->QuickSlotComponent)
	{
		ActiveRequest->QuickSlotComponent->NotifyConsumableAbilityCompleted(
			ActiveRequest->QuickSlotIndex,
			ActiveRequest->ItemSlot.ItemData);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_UseConsumableAbility::OnCancelConsumableUse(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

UPDInventoryComponent* UGA_UseConsumableAbility::ResolveInventoryComponent() const
{
	if (UPDInventoryComponent* InventoryComponent = FPDPlayerComponentResolver::ResolveInventory(GetAvatarActorFromActorInfo()))
	{
		return InventoryComponent;
	}

	return FPDPlayerComponentResolver::ResolveInventory(GetOwningActorFromActorInfo());
}

bool UGA_UseConsumableAbility::RemoveConsumableFromInventory(UPDInventoryComponent* InventoryComponent, const FPDInventorySlot& Slot) const
{
	if (!InventoryComponent || Slot.IsEmpty())
	{
		return false;
	}

	int32 InventorySlotIndex = InventoryComponent->FindSlotIndexByInstanceID(Slot.ItemInstanceID);
	if (InventorySlotIndex == INDEX_NONE)
	{
		for (int32 Index = 0; Index < InventoryComponent->Items.Num(); ++Index)
		{
			const FPDInventorySlot& Candidate = InventoryComponent->Items[Index];
			if (!Candidate.IsEmpty() && Candidate.ItemData.ItemID == Slot.ItemData.ItemID)
			{
				InventorySlotIndex = Index;
				break;
			}
		}
	}

	return InventorySlotIndex != INDEX_NONE && InventoryComponent->RemoveItemFromSlot(InventorySlotIndex, 1);
}

void UGA_UseConsumableAbility::ApplyConsumableEffect(const FPDInventorySlot& Slot)
{
	if (!Slot.ItemData.UseEffect)
	{
		return;
	}

	ApplyGameplayEffectToOwner(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		Slot.ItemData.UseEffect.GetDefaultObject(),
		GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo));
}

void UGA_UseConsumableAbility::RemoveMoveSpeedEffect()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	if (MoveSpeedEffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(MoveSpeedEffectHandle);
		MoveSpeedEffectHandle.Invalidate();
	}

	if (UseMoveSpeedEffectClass)
	{
		ASC->RemoveActiveGameplayEffectBySourceEffect(UseMoveSpeedEffectClass, ASC);
	}
}
