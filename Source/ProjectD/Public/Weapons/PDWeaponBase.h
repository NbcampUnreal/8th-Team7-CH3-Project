#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDWeaponBase.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	APDWeaponBase();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon")
	TWeakObjectPtr<AActor> WeaponOwner;

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
	void Fire();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
	void OnEquip(AActor* NewOwner);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Weapon")
	void OnUnequip();

	FORCEINLINE AActor* GetWeaponOwner() const { return WeaponOwner.Get(); }
};
