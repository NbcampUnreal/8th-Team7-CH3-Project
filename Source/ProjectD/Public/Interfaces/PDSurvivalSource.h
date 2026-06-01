#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PDSurvivalSource.generated.h"

class UGameplayEffect;

UINTERFACE(MinimalAPI)
class UPDSurvivalSource : public UInterface
{
	GENERATED_BODY()
};

class PROJECTD_API IPDSurvivalSource
{
	GENERATED_BODY()

public:
	virtual TSubclassOf<UGameplayEffect> GetHungerDecayEffectClass()   const = 0;
	virtual TSubclassOf<UGameplayEffect> GetThirstDecayEffectClass()   const = 0;
	virtual TSubclassOf<UGameplayEffect> GetStarvingEffectClass()      const = 0;
	virtual TSubclassOf<UGameplayEffect> GetDehydratedEffectClass()    const = 0;
	virtual TSubclassOf<UGameplayEffect> GetGasMaskDecayEffectClass()  const = 0;
	virtual TSubclassOf<UGameplayEffect> GetGasExposureEffectClass()   const = 0;
};
