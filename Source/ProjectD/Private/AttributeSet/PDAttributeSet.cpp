


#include "AttributeSet/PDAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffect.h"
#include "Data/PDBodyPartConfig.h"
#include "Interfaces/PDStatusEffectSource.h"
#include "Interfaces/PDSurvivalSource.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Characters/Base/PDCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
EBodyPart InferBodyPartFromHitLocation(AActor* OwnerActor, const FHitResult& HitResult)
{
	const ACharacter* Character = Cast<ACharacter>(OwnerActor);
	if (!Character || !Character->GetCapsuleComponent())
	{
		return EBodyPart::Torso;
	}

	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const float HalfHeight = FMath::Max(Capsule->GetScaledCapsuleHalfHeight(), 1.f);
	const float Radius = FMath::Max(Capsule->GetScaledCapsuleRadius(), 1.f);
	const FVector LocalImpact = Character->GetActorTransform().InverseTransformPosition(HitResult.ImpactPoint);
	const float HeightAlpha = FMath::Clamp((LocalImpact.Z + HalfHeight) / (HalfHeight * 2.f), 0.f, 1.f);

	if (HeightAlpha >= 0.82f)
	{
		return EBodyPart::Head;
	}

	if (HeightAlpha <= 0.38f)
	{
		return LocalImpact.Y >= 0.f ? EBodyPart::Leg_R : EBodyPart::Leg_L;
	}

	if (FMath::Abs(LocalImpact.Y) >= Radius * 0.45f && HeightAlpha >= 0.45f)
	{
		return LocalImpact.Y >= 0.f ? EBodyPart::Arm_R : EBodyPart::Arm_L;
	}

	return EBodyPart::Torso;
}

EBodyPart MapBodyPartFromParentBones(const USkeletalMeshComponent* Mesh, FName StartBoneName,
	const UPDBodyPartConfig* BodyPartConfig, AActor* OwnerActor)
{
	if (!Mesh || !BodyPartConfig || StartBoneName.IsNone())
	{
		return EBodyPart::None;
	}

	FName ParentBoneName = Mesh->GetParentBone(StartBoneName);
	while (!ParentBoneName.IsNone())
	{
		const EBodyPart ParentMappedPart = BodyPartConfig->GetBodyPartFromName(ParentBoneName);

		if (ParentMappedPart != EBodyPart::None)
		{
			return ParentMappedPart;
		}

		ParentBoneName = Mesh->GetParentBone(ParentBoneName);
	}

	return EBodyPart::None;
}
}

#define ATTRIBUTE_REPNOTIFY_IMPL(PropertyName) \
void UPDAttributeSet::OnRep_##PropertyName(const FGameplayAttributeData& OldValue) \
{ \
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPDAttributeSet, PropertyName, OldValue); \
}

ATTRIBUTE_REPNOTIFY_IMPL(HeadHP)
ATTRIBUTE_REPNOTIFY_IMPL(MaxHeadHP)
ATTRIBUTE_REPNOTIFY_IMPL(TorsoHP)
ATTRIBUTE_REPNOTIFY_IMPL(MaxTorsoHP)
ATTRIBUTE_REPNOTIFY_IMPL(ArmLHP)
ATTRIBUTE_REPNOTIFY_IMPL(MaxArmLHP)
ATTRIBUTE_REPNOTIFY_IMPL(ArmRHP)
ATTRIBUTE_REPNOTIFY_IMPL(MaxArmRHP)
ATTRIBUTE_REPNOTIFY_IMPL(LegLHP)
ATTRIBUTE_REPNOTIFY_IMPL(MaxLegLHP)
ATTRIBUTE_REPNOTIFY_IMPL(LegRHP)
ATTRIBUTE_REPNOTIFY_IMPL(MaxLegRHP)
ATTRIBUTE_REPNOTIFY_IMPL(Stamina)
ATTRIBUTE_REPNOTIFY_IMPL(MaxStamina)
ATTRIBUTE_REPNOTIFY_IMPL(MoveSpeed)
ATTRIBUTE_REPNOTIFY_IMPL(MaxMoveSpeed)
ATTRIBUTE_REPNOTIFY_IMPL(Hunger)
ATTRIBUTE_REPNOTIFY_IMPL(MaxHunger)
ATTRIBUTE_REPNOTIFY_IMPL(Thirst)
ATTRIBUTE_REPNOTIFY_IMPL(MaxThirst)
ATTRIBUTE_REPNOTIFY_IMPL(VisionRange)
ATTRIBUTE_REPNOTIFY_IMPL(VisionAngle)
ATTRIBUTE_REPNOTIFY_IMPL(VisionUpdateInterval)
ATTRIBUTE_REPNOTIFY_IMPL(BleedingResistance)
ATTRIBUTE_REPNOTIFY_IMPL(StaminaRegenRate)
ATTRIBUTE_REPNOTIFY_IMPL(GasMask)
ATTRIBUTE_REPNOTIFY_IMPL(MaxGasMask)

UPDAttributeSet::UPDAttributeSet()
{
	static ConstructorHelpers::FObjectFinder<UPDBodyPartConfig> BodyPartConfigAsset(
		TEXT("/Game/Main/Data/DA_BodyPartConfig.DA_BodyPartConfig"));

	if (BodyPartConfigAsset.Succeeded())
	{
		BodyPartConfig = BodyPartConfigAsset.Object;
	}
}

void UPDAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, HeadHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, MaxHeadHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, TorsoHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, MaxTorsoHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, ArmLHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, MaxArmLHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, ArmRHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, MaxArmRHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, LegLHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, MaxLegLHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, LegRHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, MaxLegRHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, MaxMoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, Hunger, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, MaxHunger, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, Thirst, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, MaxThirst, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, VisionRange, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, VisionAngle, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, VisionUpdateInterval, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, BleedingResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, StaminaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, GasMask, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDAttributeSet, MaxGasMask, COND_None, REPNOTIFY_Always);
}

void UPDAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	HandleAttributeClamp(Attribute, NewValue);
}

void UPDAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	AActor* OwnerActor = GetOwningActor();
	const bool bHasAuthority = OwnerActor && OwnerActor->HasAuthority();

	if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.f, GetMaxStamina()));
		return;
	}

	if (Data.EvaluatedData.Attribute==GetDamageAttribute())
	{
		const float LocalDamage=GetDamage();
		SetDamage(0.f);

		if (LocalDamage<=0.f) return;

		if (APDCharacterBase* DamageOwner = Cast<APDCharacterBase>(OwnerActor))
		{
			if (DamageOwner->GetLifeState() == EPDLifeState::Downed && !DamageOwner->CanBeKilledWhileDownedByDamage())
			{
				return;
			}
		}

		EBodyPart Part=EBodyPart::Torso;
		bool bMappedBodyPart = false;

		if (const FHitResult* HitResult=Data.EffectSpec.GetContext().GetHitResult())
		{

			if (BodyPartConfig)
			{
				const EBodyPart MappedPart=BodyPartConfig->GetBodyPartFromName(HitResult->BoneName);
				if (MappedPart!=EBodyPart::None)
				{
					Part=MappedPart;
					bMappedBodyPart = true;
				}
				else
				{
					const EBodyPart MyMappedPart=BodyPartConfig->GetBodyPartFromName(HitResult->MyBoneName);
					if (MyMappedPart!=EBodyPart::None)
					{
						Part=MyMappedPart;
						bMappedBodyPart = true;
					}
				}

				if (!bMappedBodyPart)
				{
					if (const USkeletalMeshComponent* HitMesh = Cast<USkeletalMeshComponent>(HitResult->GetComponent()))
					{
						EBodyPart ParentMappedPart = MapBodyPartFromParentBones(HitMesh, HitResult->BoneName, BodyPartConfig, OwnerActor);
						if (ParentMappedPart == EBodyPart::None && HitResult->MyBoneName != HitResult->BoneName)
						{
							ParentMappedPart = MapBodyPartFromParentBones(HitMesh, HitResult->MyBoneName, BodyPartConfig, OwnerActor);
						}

						if (ParentMappedPart != EBodyPart::None)
						{
							Part = ParentMappedPart;
							bMappedBodyPart = true;
						}
					}
				}
			}
			else
			{
			}

			if (!bMappedBodyPart)
			{
				Part = InferBodyPartFromHitLocation(OwnerActor, *HitResult);
			}
		}
		else
		{
		}

		const float OldHP=GetHPByPart(Part);
		const float DamageToPart = FMath::Min(LocalDamage, OldHP);
		const float OverflowDamage = LocalDamage - DamageToPart;
		const float NewHP = FMath::Max(OldHP - DamageToPart, 0.f);
		SetHPByPart(Part, NewHP);

		if (Part != EBodyPart::Torso && OverflowDamage > 0.f)
		{
			SetTorsoHP(FMath::Max(GetTorsoHP() - OverflowDamage, 0.f));
		}

		// 피격 부위/데미지 추적 — 어느 부위에 얼마가 들어갔는지 확인.
		if (const UEnum* PartEnum = StaticEnum<EBodyPart>())
		{
			UE_LOG(LogTemp, Warning, TEXT("[PD Damage Hit] %s | Part=%s | HP %.0f -> %.0f (dmg=%.0f) | mapped=%d"),
				*GetNameSafe(OwnerActor),
				*PartEnum->GetNameStringByValue(static_cast<int64>(Part)),
				OldHP, NewHP, LocalDamage, bMappedBodyPart ? 1 : 0);
		}

		if (APDCharacterBase* Owner=Cast<APDCharacterBase>(OwnerActor))
		{
			if (Owner->IsDead()) return;

			const FHitResult* CueHitResult = Data.EffectSpec.GetContext().GetHitResult();
			constexpr float HitReactCueInterval = 0.05f;
			const float CurrentTime = Owner->GetWorld() ? Owner->GetWorld()->GetTimeSeconds() : 0.f;
			if (CurrentTime - LastHitReactCueTime >= HitReactCueInterval)
			{
				LastHitReactCueTime = CurrentTime;
				if (UAbilitySystemComponent* OwnerASC = GetOwningAbilitySystemComponent())
				{
					const FHitResult DamageHitResult = CueHitResult ? *CueHitResult : FHitResult();
					const FVector HitLocation = DamageHitResult.bBlockingHit ? FVector(DamageHitResult.ImpactPoint) : Owner->GetActorLocation();
					const FVector HitNormal = DamageHitResult.bBlockingHit ? FVector(DamageHitResult.ImpactNormal) : FVector::UpVector;

					FGameplayCueParameters Params;
					Params.RawMagnitude = LocalDamage;
					Params.Location = HitLocation;
					Params.Normal = HitNormal;
					Params.Instigator = Data.EffectSpec.GetContext().GetInstigator();

					OwnerASC->ExecuteGameplayCue(PDGameplayTags::GameplayCue_Character_HitReact, Params);
				}
			}

			if (GetTorsoHP()<=0.f||GetHeadHP()<=0.f)
			{
				if (Owner->GetLifeState() == EPDLifeState::Downed)
				{
					if (Owner->CanBeKilledWhileDownedByDamage())
					{
						Owner->HandleDeath(Data.EffectSpec.GetContext().GetInstigator());
					}
					return;
				}

				if (Owner->ShouldEnterDownedStateOnLethalDamage() && Owner->GetLifeState() == EPDLifeState::Alive)
				{
					Owner->HandleDowned(Data.EffectSpec.GetContext().GetInstigator());
				}
				else
				{
					Owner->HandleDeath(Data.EffectSpec.GetContext().GetInstigator());
				}
				return;
			}
		}

		UAbilitySystemComponent* ASC=GetOwningAbilitySystemComponent();
		if (bHasAuthority)
		{
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
	}

	if (Data.EvaluatedData.Attribute == GetHungerAttribute()  ||
		Data.EvaluatedData.Attribute == GetThirstAttribute()  ||
		Data.EvaluatedData.Attribute == GetGasMaskAttribute())
	{
		if (!bIsInitialized) return;
		if (!bHasAuthority) return;
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
	else if (Attribute==GetMoveSpeedAttribute())
	{
		const float MaxSpeed = GetMaxMoveSpeed();
		if (MaxSpeed > KINDA_SMALL_NUMBER)
		{
			NewValue = FMath::Clamp(NewValue, MaxSpeed * 0.2f, MaxSpeed);
		}
	}
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
