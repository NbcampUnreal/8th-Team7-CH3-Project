#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/PDDamageable.h"
#include "Interfaces/PDInteractable.h"
#include "Interfaces/PDStatusEffectSource.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "PDCharacterBase.generated.h"

class UPDAttributeSet;
class UGameplayAbility;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeathSignature, AActor*, Killer);
class UGameplayEffect;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDCharacterBase : public ACharacter,
										public IPDDamageable,
										public IPDInteractable,
										public IAbilitySystemInterface,
										public IPDStatusEffectSource
{
	GENERATED_BODY()

public:
	APDCharacterBase();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon")
	FName WeaponSocketName=TEXT("WeaponSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|GAS")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditAnywhere, Category = "PD|GAS")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	UPROPERTY(EditAnywhere, Category = "PD|GAS")
	TArray<TSubclassOf<UGameplayAbility>> ActiveAbilities;
	
	UPROPERTY(EditAnywhere, Category = "PD|GAS")
	TSubclassOf<UGameplayEffect> DefaultAttributes;

	UPROPERTY(EditDefaultsOnly, Category = "PD|StatusEffect")
	TSubclassOf<UGameplayEffect> LegDamagedEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "PD|StatusEffect")
	TSubclassOf<UGameplayEffect> LegCrippledEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "PD|StatusEffect")
	TSubclassOf<UGameplayEffect> BleedingEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "PD|StatusEffect")
	TSubclassOf<UGameplayEffect> ArmDamagedEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "PD|StatusEffect")
	TSubclassOf<UGameplayEffect> ArmCrippledEffectClass;

	void GiveStartupAbilities();
	void GiveActiveAbilities();
	void InitializeAttributes();

public:
	UPROPERTY(BlueprintAssignable, Category = "PD|Damage")
	FOnDeathSignature OnDeathDelegate;

	virtual void ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo) override;
	virtual float GetCurrentHealth_Implementation() const override;
	virtual float GetMaxHealth_Implementation() const override;
	virtual bool IsAlive_Implementation() const override;
	virtual void Interact_Implementation(AActor* Interactor) override {}
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return ASC; }

	// IPDStatusEffectSource
	virtual TSubclassOf<UGameplayEffect> GetLegDamagedEffectClass()  const override { return LegDamagedEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetLegCrippledEffectClass() const override { return LegCrippledEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetArmDamagedEffectClass()  const override { return ArmDamagedEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetArmCrippledEffectClass() const override { return ArmCrippledEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetBleedingEffectClass()    const override { return BleedingEffectClass; }

	UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
	void AttachActorToWeaponSocket(AActor* ActorToAttach);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Damage")
	void OnDeath(AActor* Killer);

	virtual void HandleDeath(AActor* Killer);

protected:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY()
	TObjectPtr<UPDAttributeSet> AttributeSet;

	virtual void InitAbilitySystem();
	virtual void BeginPlay() override;

private:
	void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);
};
