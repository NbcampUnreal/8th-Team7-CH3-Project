#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "Interfaces/PDDamageable.h"
#include "Interfaces/PDInteractable.h"
#include "Interfaces/PDStatusEffectSource.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "PDCharacterBase.generated.h"

class UPDAttributeSet;
class UGameplayAbility;
class UAIPerceptionStimuliSourceComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeathSignature, AActor*, Killer);
class UGameplayEffect;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDCharacterBase : public ACharacter,
										public IPDDamageable,
										public IPDInteractable,
										public IAbilitySystemInterface,
										public IPDStatusEffectSource,
										public IGenericTeamAgentInterface
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

	/** 본 캐릭터의 팀. 디자이너가 BP 디폴트 또는 자식 생성자에서 지정 (1=Player, 2=Hostile, 255=Neutral). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|AI")
	uint8 TeamID = FGenericTeamId::NoTeam;

	/** 다른 AI가 본 캐릭터를 자극(시각/청각)으로 인지할 수 있도록 등록. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionStimuliSourceComponent> StimuliSource;

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

	// 엔진 perception 의 affiliation 시스템에 TeamID 노출.
	// AAIController 가 possess 한 폰의 인터페이스로 자동 위임.
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(TeamID); }

	UFUNCTION(BlueprintPure, Category = "PD|AI")
	FORCEINLINE UAIPerceptionStimuliSourceComponent* GetStimuliSource() const { return StimuliSource; }

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
