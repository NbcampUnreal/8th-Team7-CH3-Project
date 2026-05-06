#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Type/Types.h"
#include "PDDamageable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPDDamageable : public UInterface
{
	GENERATED_BODY()
};

class PROJECTD_API IPDDamageable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "PD|Damage")
	void ApplyDamage(const FPDDamageInfo& DamageInfo);

	UFUNCTION(BlueprintNativeEvent, Category = "PD|Damage")
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintNativeEvent, Category = "PD|Damage")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintNativeEvent, Category = "PD|Damage")
	bool IsAlive() const;
};
