// Fill out your copyright notice in the Description page of Project Settings.


#include "AttributeSet/PDAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffect.h"
#include "Data/PDBodyPartConfig.h"
#include "Interfaces/PDStatusEffectSource.h"
#include "Interfaces/PDSurvivalSource.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Characters/Base/PDCharacterBase.h"

void UPDAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	HandleAttributeClamp(Attribute, NewValue);
}

void UPDAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute==GetDamageAttribute())
	{
		UE_LOG(LogTemp, Warning, TEXT("Damage Source GE: %s | Amount: %f"),
		*Data.EffectSpec.Def->GetName(), GetDamage());
		const float LocalDamage=GetDamage();
		SetDamage(0.f);

		if (LocalDamage<=0.f) return;
		
		EBodyPart Part=EBodyPart::Torso;
		if (const FHitResult* HitResult=Data.EffectSpec.GetContext().GetHitResult())
		{
			if (BodyPartConfig)
			{
				Part=BodyPartConfig->GetBodyPartFromName(HitResult->BoneName);
			}
		}
		const float NewHP=FMath::Max(GetHPByPart(Part)-LocalDamage, 0.f);
		SetHPByPart(Part, NewHP);
		
		AActor* OwnerActor=GetOwningActor();
		
		if (APDCharacterBase* Owner=Cast<APDCharacterBase>(OwnerActor))
		{
			if (GetTorsoHP()<=0.f||GetHeadHP()<=0.f)
			{
				Owner->HandleDeath(Data.EffectSpec.GetContext().GetInstigator());
				return;
			}
			// 살아있을 때만 HitReact
			Owner->OnDamageTaken();
		}
		
		UAbilitySystemComponent* ASC=GetOwningAbilitySystemComponent();
		if (IPDStatusEffectSource* Source=Cast<IPDStatusEffectSource>(OwnerActor))
		{
			const float Resistance=FMath::Clamp(GetBleedingResistance(), 0.f, 100.f);
			const float BleedChance=0.2f*(1.f-Resistance/100.f);

			if (FMath::FRand()<BleedChance)
			{
				TryApplyInjuryEffect(ASC, Source->GetBleedingEffectClass(), PDGameplayTags::State_Debuff_Bleeding);
			}
			CheckAndApplyInjuryEffects(ASC, Source);
		}
	}
	
	if (Data.EvaluatedData.Attribute == GetHungerAttribute()  ||
		Data.EvaluatedData.Attribute == GetThirstAttribute()  ||
		Data.EvaluatedData.Attribute == GetGasMaskAttribute())
	{
		if (!bIsInitialized) return;
		AActor* OwnerActor=GetOwningActor();
		UAbilitySystemComponent* ASC=GetOwningAbilitySystemComponent();
		if (IPDSurvivalSource* Survival=Cast<IPDSurvivalSource>(OwnerActor))
		{
			CheckAndApplySurvivalEffects(ASC, Survival);
		}
	}
}

void UPDAttributeSet::HandleAttributeClamp(const FGameplayAttribute& Attribute, float& NewValue) const
{
	NewValue=FMath::Max(NewValue, 0.f);

	if (Attribute==GetHeadHPAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxHeadHP());
	else if (Attribute==GetTorsoHPAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxTorsoHP());
	else if (Attribute==GetArmLHPAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxArmLHP());
	else if (Attribute==GetArmRHPAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxArmRHP());
	else if (Attribute==GetLegLHPAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxLegLHP());
	else if (Attribute==GetLegRHPAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxLegRHP());
	else if (Attribute==GetStaminaAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	else if (Attribute==GetMoveSpeedAttribute()) NewValue=FMath::Clamp(NewValue, GetMaxMoveSpeed()*0.2f, GetMaxMoveSpeed());
	else if (Attribute==GetThirstAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxThirst());
	else if (Attribute==GetHungerAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxHunger());
	else if (Attribute==GetGasMaskAttribute()) NewValue=FMath::Clamp(NewValue, 0.f, GetMaxGasMask());

}

FGameplayAttribute UPDAttributeSet::GetAttributeByPart(EBodyPart Part) const
{
	switch (Part)
	{
		case EBodyPart::Head:  return GetHeadHPAttribute();
		case EBodyPart::Arm_L: return GetArmLHPAttribute();
		case EBodyPart::Arm_R: return GetArmRHPAttribute();
		case EBodyPart::Leg_L: return GetLegLHPAttribute();
		case EBodyPart::Leg_R: return GetLegRHPAttribute();
		default:               return GetTorsoHPAttribute();
	}
}
void UPDAttributeSet::ApplyDamageToPart(EBodyPart Part, float DamageAmount)
{
	switch (Part)
	{
		case EBodyPart::Head:
			SetHeadHP(FMath::Max(GetHeadHP() - DamageAmount, 0.f));
			break;
		case EBodyPart::Arm_L:
			SetArmLHP(FMath::Max(GetArmLHP() - DamageAmount, 0.f));
			break;
		case EBodyPart::Arm_R:
			SetArmRHP(FMath::Max(GetArmRHP() - DamageAmount, 0.f));
			break;
		case EBodyPart::Leg_L:
			SetLegLHP(FMath::Max(GetLegLHP() - DamageAmount, 0.f));
			break;
		case EBodyPart::Leg_R:
			SetLegRHP(FMath::Max(GetLegRHP() - DamageAmount, 0.f));
			break;
		case EBodyPart::Torso:
		default:
			SetTorsoHP(FMath::Max(GetTorsoHP() - DamageAmount, 0.f));
			break;
	}
}

float UPDAttributeSet::GetHPByPart(EBodyPart Part) const
{
	switch (Part)
	{
		case EBodyPart::Head:  return GetHeadHP();
		case EBodyPart::Torso: return GetTorsoHP();
		case EBodyPart::Arm_L: return GetArmLHP();
		case EBodyPart::Arm_R: return GetArmRHP();
		case EBodyPart::Leg_L: return GetLegLHP();
		case EBodyPart::Leg_R: return GetLegRHP();
		default:               return 0.f;
	}
}

void UPDAttributeSet::SetHPByPart(EBodyPart Part, float NewValue)
{
	switch (Part)
	{
		case EBodyPart::Head:  SetHeadHP(NewValue);  break;
		case EBodyPart::Torso: SetTorsoHP(NewValue); break;
		case EBodyPart::Arm_L: SetArmLHP(NewValue);  break;
		case EBodyPart::Arm_R: SetArmRHP(NewValue);  break;
		case EBodyPart::Leg_L: SetLegLHP(NewValue);  break;
		case EBodyPart::Leg_R: SetLegRHP(NewValue);  break;
	}
}

void UPDAttributeSet::CheckAndApplyInjuryEffects(UAbilitySystemComponent* ASC, IPDStatusEffectSource* Source)
{
	if (!ASC||!Source) return;
	
	const bool bLeftLegDown=GetLegLHP()<=0.f;
	const bool bRightLegDown=GetLegRHP()<=0.f;

	if (bLeftLegDown&&bRightLegDown)
	{
		TryApplyInjuryEffect(ASC, Source->GetLegCrippledEffectClass(), PDGameplayTags::State_Debuff_LegCrippled);
	}
	else if (bLeftLegDown||bRightLegDown)
	{
		TryApplyInjuryEffect(ASC, Source->GetLegDamagedEffectClass(), PDGameplayTags::State_Debuff_LegDamaged);
	}
	
	const bool bLeftArmDown=GetArmLHP()<=0.f;
	const bool bRightArmDown=GetArmRHP()<=0.f;

	if (bLeftArmDown&&bRightArmDown)
	{
		TryApplyInjuryEffect(ASC, Source->GetArmCrippledEffectClass(), PDGameplayTags::State_Debuff_ArmCrippled);
	}
	else if (bLeftArmDown||bRightArmDown)
	{
		TryApplyInjuryEffect(ASC, Source->GetArmDamagedEffectClass(), PDGameplayTags::State_Debuff_ArmDamaged);
	}
}

void UPDAttributeSet::TryApplyInjuryEffect(UAbilitySystemComponent* ASC,
	TSubclassOf<UGameplayEffect> EffectClass, const FGameplayTag& GuardTag)
{
	if (!EffectClass||ASC->HasMatchingGameplayTag(GuardTag)) return;
	
	UE_LOG(LogTemp, Warning, TEXT("TryApplyInjuryEffect: %s"), *GuardTag.ToString());
	
	FGameplayEffectContextHandle Context=ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec=ASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
	if (Spec.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void UPDAttributeSet::CheckAndApplySurvivalEffects(UAbilitySystemComponent* ASC, IPDSurvivalSource* Source)
{
	if (!ASC || !Source) return;

	TryApplyOrRemoveSurvivalEffect(ASC, Source->GetStarvingEffectClass(),
		PDGameplayTags::State_Debuff_Starving, GetHunger() <= 0.f);

	TryApplyOrRemoveSurvivalEffect(ASC, Source->GetDehydratedEffectClass(),
		PDGameplayTags::State_Debuff_Dehydrated, GetThirst() <= 0.f);

	TryApplyOrRemoveSurvivalEffect(ASC, Source->GetGasExposureEffectClass(),
		PDGameplayTags::State_Debuff_GasExposure, GetGasMask() <= 0.f);
}

void UPDAttributeSet::TryApplyOrRemoveSurvivalEffect(UAbilitySystemComponent* ASC,
	TSubclassOf<UGameplayEffect> EffectClass, const FGameplayTag& GuardTag, bool bShouldApply)
{
	if (!ASC || !EffectClass) return;

	const bool bHasTag = ASC->HasMatchingGameplayTag(GuardTag);

	if (bShouldApply && !bHasTag)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
		if (Spec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
	else if (!bShouldApply && bHasTag)
	{
		FGameplayTagContainer TagContainer;
		TagContainer.AddTag(GuardTag);
		ASC->RemoveActiveEffectsWithTags(TagContainer);
	}
}
