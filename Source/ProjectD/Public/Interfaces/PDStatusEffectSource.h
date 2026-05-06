#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PDStatusEffectSource.generated.h"

class UGameplayEffect;

UINTERFACE(MinimalAPI)
class UPDStatusEffectSource : public UInterface
{
	GENERATED_BODY()
};


class PROJECTD_API IPDStatusEffectSource
{
	GENERATED_BODY()

public:
	virtual TSubclassOf<UGameplayEffect> GetLegDamagedEffectClass()  const = 0;
	virtual TSubclassOf<UGameplayEffect> GetLegCrippledEffectClass() const = 0;
	virtual TSubclassOf<UGameplayEffect> GetArmDamagedEffectClass()  const = 0;
	virtual TSubclassOf<UGameplayEffect> GetArmCrippledEffectClass() const = 0;
	virtual TSubclassOf<UGameplayEffect> GetBleedingEffectClass() const = 0;
};
