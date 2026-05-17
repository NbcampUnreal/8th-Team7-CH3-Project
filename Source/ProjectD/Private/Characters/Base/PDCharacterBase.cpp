#include "Characters/Base/PDCharacterBase.h"
#include "Animation/PDAnimInstance.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Ability/PDGameplayAbilityBase.h"
#include "AttributeSet/PDAttributeSet.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/PDGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Type/Types.h"

APDCharacterBase::APDCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	ASC=CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	AttributeSet=CreateDefaultSubobject<UPDAttributeSet>(TEXT("AttributeSet"));

	// 자극원 컴포넌트
	StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
	if (StimuliSource)
	{
		StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
		StimuliSource->RegisterForSense(UAISense_Hearing::StaticClass());
		StimuliSource->bAutoRegister = true;
	}
	// 무기 컴포넌트
	WeaponComponent = CreateDefaultSubobject<UPDWeaponComponent>(TEXT("WeaponComponent"));
}

void APDCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	InitAbilitySystem();

}

void APDCharacterBase::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed=Data.NewValue;
}

void APDCharacterBase::InitAbilitySystem()
{
	if (!ASC) return;
	ASC->InitAbilityActorInfo(this, this);
	ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetMoveSpeedAttribute())
		.AddUObject(this, &APDCharacterBase::OnMoveSpeedChanged);
	
	InitializeAttributes();
	GiveStartupAbilities();
	GiveActiveAbilities();
}

void APDCharacterBase::InitializeAttributes()
{
	if (!ASC || !DefaultAttributes)
	{
		UE_LOG(LogTemp, Warning, TEXT("InitializeAttributes: ASC=%d, DefaultAttributes=%d"), 
			ASC != nullptr, DefaultAttributes != nullptr);
		return;
	}
	FGameplayEffectContextHandle ContextHandle=ASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle=ASC->MakeOutgoingSpec(DefaultAttributes, 1.f, ContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	if (AttributeSet)
	{
		AttributeSet->SetHeadHP (AttributeSet->GetMaxHeadHP());
		AttributeSet->SetTorsoHP(AttributeSet->GetMaxTorsoHP());
		AttributeSet->SetArmLHP (AttributeSet->GetMaxArmLHP());
		AttributeSet->SetArmRHP (AttributeSet->GetMaxArmRHP());
		AttributeSet->SetLegLHP (AttributeSet->GetMaxLegLHP());
		AttributeSet->SetLegRHP (AttributeSet->GetMaxLegRHP());
		AttributeSet->SetStamina(AttributeSet->GetMaxStamina());
		AttributeSet->SetMoveSpeed(AttributeSet->GetMaxMoveSpeed());
		AttributeSet->SetHunger(AttributeSet->GetMaxHunger());
		AttributeSet->SetThirst(AttributeSet->GetMaxThirst());
		AttributeSet->SetGasMask(AttributeSet->GetMaxGasMask());
		AttributeSet->bIsInitialized=true;
	}
}

void APDCharacterBase::GiveStartupAbilities()
{
	if (!ASC) return;
	for (const auto& StartupAbility : StartupAbilities)
	{
		FGameplayAbilitySpec AbilitySpec(StartupAbility);
		ASC->GiveAbility(AbilitySpec);
	}
}

void APDCharacterBase::GiveActiveAbilities()
{
	if (!ASC) return;
	for (const auto& ActiveAbility : ActiveAbilities)
	{
		FGameplayAbilitySpec AbilitySpec(ActiveAbility);
		ASC->GiveAbility(AbilitySpec);
	}
}

void APDCharacterBase::ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo)
{
	if (!ASC) return;

	FGameplayEffectContextHandle Context=ASC->MakeEffectContext();
	Context.AddHitResult(DamageInfo.HitResult);
	Context.AddInstigator(DamageInfo.Instigator.Get(), DamageInfo.Instigator.Get());

	FGameplayEffectSpecHandle Spec=ASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
	if (!Spec.IsValid()) return;

	Spec.Data->SetSetByCallerMagnitude(PDGameplayTags::Data_Damage, DamageInfo.BaseDamage);
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

float APDCharacterBase::GetCurrentHealth_Implementation() const
{
	if (AttributeSet) return AttributeSet->GetTorsoHP();
	return 0.f;
}

float APDCharacterBase::GetMaxHealth_Implementation() const
{
	if (AttributeSet) return AttributeSet->GetMaxTorsoHP();
	return 0.f;
}

bool APDCharacterBase::IsAlive_Implementation() const
{
	return GetCurrentHealth_Implementation() > 0.f;
}

void APDCharacterBase::HandleDeath(AActor* Killer)
{
	// 자극원 해제.
	if (StimuliSource)
	{
		StimuliSource->UnregisterFromPerceptionSystem();
	}

	OnDeathDelegate.Broadcast(Killer);
	OnDeath(Killer);

	if (APlayerController* PC=Cast<APlayerController>(GetController()))
	{
		if (APDGameMode* GM=GetWorld()->GetAuthGameMode<APDGameMode>())
		{
			GM->OnPlayerDied(PC, Killer);
		}
	}
}

void APDCharacterBase::OnDamageTaken()
{
	if (UPDAnimInstance* AI = Cast<UPDAnimInstance>(GetMesh()->GetAnimInstance()))
		AI->PlayHitReaction();
}

void APDCharacterBase::AttachActorToWeaponSocket(AActor* ActorToAttach)
{
	if (!ActorToAttach) return;

	ActorToAttach->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		WeaponSocketName
	);
}
