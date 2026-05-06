// Fill out your copyright notice in the Description page of Project Settings.

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
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData HeadHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, HeadHP)
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxHeadHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxHeadHP)

	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData TorsoHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, TorsoHP)
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxTorsoHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxTorsoHP)

	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData ArmLHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, ArmLHP)
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxArmLHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxArmLHP)

	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData ArmRHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, ArmRHP)
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxArmRHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxArmRHP)

	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData LegLHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, LegLHP)
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxLegLHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxLegLHP)

	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData LegRHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, LegRHP)
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData MaxLegRHP;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxLegRHP)
	
	UPROPERTY(BlueprintReadOnly, Category = "PD|Stamina")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, Stamina)
	UPROPERTY(BlueprintReadOnly, Category = "PD|Stamina")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxStamina)
	
	UPROPERTY(BlueprintReadOnly, Category="PD|Movement")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MoveSpeed)
	UPROPERTY(BlueprintReadOnly, Category ="PD|Movement")
	FGameplayAttributeData MaxMoveSpeed;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxMoveSpeed)
	
	UPROPERTY(BlueprintReadOnly, Category="PD|Hunger")
	FGameplayAttributeData Hunger;     
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, Hunger)
	UPROPERTY(BlueprintReadOnly, Category="PD|Hunger")
	FGameplayAttributeData MaxHunger;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxHunger)

	UPROPERTY(BlueprintReadOnly, Category="PD|Thirst")
	FGameplayAttributeData Thirst;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, Thirst)
	UPROPERTY(BlueprintReadOnly, Category="PD|Thirst")
	FGameplayAttributeData MaxThirst;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, MaxThirst)
	
	UPROPERTY(BlueprintReadOnly, Category="PD|Vision")
	FGameplayAttributeData VisionRange;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, VisionRange)

	UPROPERTY(BlueprintReadOnly, Category="PD|Vision")
	FGameplayAttributeData VisionAngle;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, VisionAngle)

	UPROPERTY(BlueprintReadOnly, Category="PD|Vision")
	FGameplayAttributeData VisionUpdateInterval;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, VisionUpdateInterval)
	
	UPROPERTY(BlueprintReadOnly, Category = "PD|StatusEffect")
	FGameplayAttributeData BleedingResistance;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, BleedingResistance)
	
	//Meta Attribute
	UPROPERTY(BlueprintReadOnly, Category = "PD|Meta")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(UPDAttributeSet, Damage)
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UPDBodyPartConfig> BodyPartConfig;

	bool bIsInitialized=false;
protected:
	//Pre -> Clamping 
	//Post -> GE (Death, Apply Damage)
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	

private:
	void HandleAttributeClamp(const FGameplayAttribute& Attribute, float& NewValue) const;
	void ApplyDamageToPart(EBodyPart Part, float DamageAmount);

	float GetHPByPart(EBodyPart Part) const;
	void SetHPByPart(EBodyPart Part, float NewValue);

	FGameplayAttribute GetAttributeByPart(EBodyPart Part) const;

	// 부상 GE 적용 — PostGameplayEffectExecute 캡슐화
	void CheckAndApplyInjuryEffects(UAbilitySystemComponent* ASC, IPDStatusEffectSource* Source);
	void TryApplyInjuryEffect(UAbilitySystemComponent* ASC,
		TSubclassOf<UGameplayEffect> EffectClass, const FGameplayTag& GuardTag);

	// 생존 GE 적용/해제 — IPDSurvivalSource 전용
	void CheckAndApplySurvivalEffects(UAbilitySystemComponent* ASC, IPDSurvivalSource* Source);
	void TryApplyOrRemoveSurvivalEffect(UAbilitySystemComponent* ASC,
		TSubclassOf<UGameplayEffect> EffectClass, const FGameplayTag& GuardTag, bool bShouldApply);
};
