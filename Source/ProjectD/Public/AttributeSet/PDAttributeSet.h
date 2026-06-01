

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Type/Types.h"
#include "PDAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

class UPDBodyPartConfig;
class IPDStatusEffectSource;
class IPDSurvivalSource;

UCLASS()
class PROJECTD_API UPDAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	UPDAttributeSet();

	UPROPERTY(ReplicatedUsing=OnRep_HeadHP, BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData HeadHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, HeadHP)
	UPROPERTY(ReplicatedUsing=OnRep_MaxHeadHP, BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxHeadHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxHeadHP)

	UPROPERTY(ReplicatedUsing=OnRep_TorsoHP, BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData TorsoHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, TorsoHP)
	UPROPERTY(ReplicatedUsing=OnRep_MaxTorsoHP, BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxTorsoHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxTorsoHP)

	UPROPERTY(ReplicatedUsing=OnRep_ArmLHP, BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData ArmLHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, ArmLHP)
	UPROPERTY(ReplicatedUsing=OnRep_MaxArmLHP, BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxArmLHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxArmLHP)

	UPROPERTY(ReplicatedUsing=OnRep_ArmRHP, BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData ArmRHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, ArmRHP)
	UPROPERTY(ReplicatedUsing=OnRep_MaxArmRHP, BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxArmRHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxArmRHP)

	UPROPERTY(ReplicatedUsing=OnRep_LegLHP, BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData LegLHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, LegLHP)
	UPROPERTY(ReplicatedUsing=OnRep_MaxLegLHP, BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxLegLHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxLegLHP)

	UPROPERTY(ReplicatedUsing=OnRep_LegRHP, BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData LegRHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, LegRHP)
	UPROPERTY(ReplicatedUsing=OnRep_MaxLegRHP, BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxLegRHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxLegRHP)

	UPROPERTY(ReplicatedUsing=OnRep_Stamina, BlueprintReadOnly, Category = "PD|Stamina")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, Stamina)
	UPROPERTY(ReplicatedUsing=OnRep_MaxStamina, BlueprintReadOnly, Category = "PD|Stamina")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxStamina)

	UPROPERTY(ReplicatedUsing=OnRep_MoveSpeed, BlueprintReadOnly, Category="PD|Movement")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MoveSpeed)
	UPROPERTY(ReplicatedUsing=OnRep_MaxMoveSpeed, BlueprintReadOnly, Category ="PD|Movement")
	FGameplayAttributeData MaxMoveSpeed;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxMoveSpeed)

	UPROPERTY(ReplicatedUsing=OnRep_Hunger, BlueprintReadOnly, Category="PD|Hunger")
	FGameplayAttributeData Hunger;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, Hunger)
	UPROPERTY(ReplicatedUsing=OnRep_MaxHunger, BlueprintReadOnly, Category="PD|Hunger")
	FGameplayAttributeData MaxHunger;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxHunger)

	UPROPERTY(ReplicatedUsing=OnRep_Thirst, BlueprintReadOnly, Category="PD|Thirst")
	FGameplayAttributeData Thirst;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, Thirst)
	UPROPERTY(ReplicatedUsing=OnRep_MaxThirst, BlueprintReadOnly, Category="PD|Thirst")
	FGameplayAttributeData MaxThirst;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxThirst)

	UPROPERTY(ReplicatedUsing=OnRep_VisionRange, BlueprintReadOnly, Category="PD|Vision")
	FGameplayAttributeData VisionRange;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, VisionRange)

	UPROPERTY(ReplicatedUsing=OnRep_VisionAngle, BlueprintReadOnly, Category="PD|Vision")
	FGameplayAttributeData VisionAngle;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, VisionAngle)

	UPROPERTY(ReplicatedUsing=OnRep_VisionUpdateInterval, BlueprintReadOnly, Category="PD|Vision")
	FGameplayAttributeData VisionUpdateInterval;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, VisionUpdateInterval)

	UPROPERTY(ReplicatedUsing=OnRep_BleedingResistance, BlueprintReadOnly, Category = "PD|StatusEffect")
	FGameplayAttributeData BleedingResistance;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, BleedingResistance)


	UPROPERTY(ReplicatedUsing=OnRep_StaminaRegenRate, BlueprintReadOnly, Category="PD|Stamina")
	FGameplayAttributeData StaminaRegenRate;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, StaminaRegenRate)

	UPROPERTY(ReplicatedUsing=OnRep_GasMask, BlueprintReadOnly, Category="PD|GasMask")
	FGameplayAttributeData GasMask;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, GasMask)
	UPROPERTY(ReplicatedUsing=OnRep_MaxGasMask, BlueprintReadOnly, Category="PD|GasMask")
	FGameplayAttributeData MaxGasMask;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxGasMask)


	UPROPERTY(BlueprintReadOnly, Category = "PD|Meta")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, Damage)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Damage")
	TObjectPtr<UPDBodyPartConfig> BodyPartConfig;

	bool bIsInitialized=false;
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UFUNCTION()
	void OnRep_HeadHP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxHeadHP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_TorsoHP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxTorsoHP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_ArmLHP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxArmLHP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_ArmRHP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxArmRHP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_LegLHP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxLegLHP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_LegRHP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxLegRHP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxMoveSpeed(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Hunger(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxHunger(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Thirst(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxThirst(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_VisionRange(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_VisionAngle(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_VisionUpdateInterval(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_BleedingResistance(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_GasMask(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxGasMask(const FGameplayAttributeData& OldValue);


private:
	float LastHitReactCueTime = -1.0e30f;

	void HandleAttributeClamp(const FGameplayAttribute& Attribute, float& NewValue) const;
	void ApplyDamageToPart(EBodyPart Part, float DamageAmount);

	float GetHPByPart(EBodyPart Part) const;
	void SetHPByPart(EBodyPart Part, float NewValue);

	FGameplayAttribute GetAttributeByPart(EBodyPart Part) const;


	void CheckAndApplyInjuryEffects(UAbilitySystemComponent* ASC, IPDStatusEffectSource* Source);
	void TryApplyInjuryEffect(UAbilitySystemComponent* ASC,
		TSubclassOf<UGameplayEffect> EffectClass, const FGameplayTag& GuardTag);


	void CheckAndApplySurvivalEffects(UAbilitySystemComponent* ASC, IPDSurvivalSource* Source);
	void TryApplyOrRemoveSurvivalEffect(UAbilitySystemComponent* ASC,
		TSubclassOf<UGameplayEffect> EffectClass, const FGameplayTag& GuardTag, bool bShouldApply);
};
