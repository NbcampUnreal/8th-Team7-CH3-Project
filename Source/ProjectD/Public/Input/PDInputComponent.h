
#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "Input/PDInputConfig.h"
#include "PDInputComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	//이동, 시야 전환, GAS안 거치고 즉각 실행되는 기본 동작들 여기에 바인딩 하면 됨.
	template<class UserClass, typename FuncType>
	void BindNativeAction(const UPDInputConfig* InputConfig, const FGameplayTag& InputTag,
						  ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func)
	{
		if (const UInputAction* IA=InputConfig->FindNativeInputActionForTag(InputTag))
		{
			BindAction(IA, TriggerEvent, Object, Func);
		}
	}
	
	//사격, 스킬, GAS Ability 필요한 동작들 여기서 활용.(Started, Completed) 필요하다면 release같은거 추가하면 됩니다.
	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const UPDInputConfig* InputConfig, UserClass* Object,
							PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc)
	{
		for (const FPDInputAction& Entry:InputConfig->AbilityInputActions)
		{
			if (!Entry.InputAction||!Entry.InputTag.IsValid()) continue;

			BindAction(Entry.InputAction, ETriggerEvent::Started, Object, PressedFunc, Entry.InputTag);
			BindAction(Entry.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Entry.InputTag);
		}
	}
};
