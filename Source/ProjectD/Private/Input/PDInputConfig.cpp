#include "Input/PDInputConfig.h"

const UInputAction* UPDInputConfig::FindNativeInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FPDInputAction& Action:NativeInputActions)
	{
		if (Action.InputAction&&Action.InputTag==InputTag) return Action.InputAction;
	}
	return nullptr;
}

const UInputAction* UPDInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FPDInputAction& Action:AbilityInputActions)
	{
		if (Action.InputAction&&Action.InputTag==InputTag) return Action.InputAction;
	}
	return nullptr;
}
