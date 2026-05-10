#pragma once

#include "CoreMinimal.h"
#include "PDAnimInstance.h"
#include "PDAnimLayerBase.generated.h"

UCLASS()
class PROJECTD_API UPDAnimLayerBase : public UPDAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override {}
	virtual void NativeUpdateAnimation(float DeltaSeconds) override {}
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override {}
};
