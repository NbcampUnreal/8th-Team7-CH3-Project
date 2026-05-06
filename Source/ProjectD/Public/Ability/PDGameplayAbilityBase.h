// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "PDGameplayAbilityBase.generated.h"

class UPDAttributeSet;
class APDPlayerController;
class APDCharacterBase;
/**
 * 
 */
UCLASS()
class PROJECTD_API UPDGameplayAbilityBase : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UPDGameplayAbilityBase();
	
	UPROPERTY(EditDefaultsOnly, Category="PD|Ability")
	FGameplayTag StartupInputTag;
	
protected:
	UFUNCTION(BlueprintCallable, Category="PD|Ability")
	APDCharacterBase* GetPDCharacter() const;
	
	UFUNCTION(BlueprintCallable, Category="PD|Ability")
	APDPlayerController* GetPDPlayerController() const;

	UFUNCTION(BlueprintCallable, Category="PD|Ability")
	const UPDAttributeSet* GetAttributeSet() const;
	
	UFUNCTION(BlueprintCallable, Category="PD|Ability")
	void PlayAbilityMontage(UAnimMontage* Montage, float PlayRate=1.f);
	
	
};
