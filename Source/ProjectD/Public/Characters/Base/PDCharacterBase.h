#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "Interfaces/PDDamageable.h"
#include "Interfaces/PDInteractable.h"
#include "Interfaces/PDStatusEffectSource.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Component/PDWeaponComponent.h"
#include "PDCharacterBase.generated.h"

class UPDAttributeSet;
class UGameplayAbility;
class UAIPerceptionStimuliSourceComponent;
class FLifetimeProperty;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeathSignature, AActor*, Killer);
class UGameplayEffect;

UENUM(BlueprintType)
enum class EPDLifeState : uint8
{
	Alive = 0,
	Downed = 1,
	GettingUp = 3,
	Dead = 2
};

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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Downed")
	float DownedMoveSpeed = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Downed", meta=(ClampMin="0.0", ForceUnits="s"))
	float DownedLifeSpan = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Downed", meta=(ClampMin="0.0", ForceUnits="s"))
	float DownedDamageGracePeriod = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Revive", meta=(ClampMin="0.0"))
	float ReviveTime = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Revive", meta=(ClampMin="0.0"))
	float ReviveInteractDistance = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Revive", meta=(ClampMin="0.01"))
	float ReviveValidationInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Revive", meta=(ClampMin="0.0"))
	float ReviveCancelMoveTolerance = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Revive")
	TSubclassOf<UGameplayEffect> ReviveEffectClass;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|AI")
	uint8 TeamID = FGenericTeamId::NoTeam;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionStimuliSourceComponent> StimuliSource;

	void GiveStartupAbilities();
	void GiveActiveAbilities();
	void InitializeAttributes();

public:
	UPROPERTY(BlueprintAssignable, Category = "PD|Damage")
	FOnDeathSignature OnDeathDelegate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Weapon",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDWeaponComponent> WeaponComponent;

	virtual void ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo) override;
	virtual float GetCurrentHealth_Implementation() const override;
	virtual float GetMaxHealth_Implementation() const override;
	virtual bool IsAlive_Implementation() const override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return ASC; }



	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(TeamID); }

	UFUNCTION(BlueprintPure, Category = "PD|AI")
	FORCEINLINE UAIPerceptionStimuliSourceComponent* GetStimuliSource() const { return StimuliSource; }


	virtual TSubclassOf<UGameplayEffect> GetLegDamagedEffectClass()  const override { return LegDamagedEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetLegCrippledEffectClass() const override { return LegCrippledEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetArmDamagedEffectClass()  const override { return ArmDamagedEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetArmCrippledEffectClass() const override { return ArmCrippledEffectClass; }
	virtual TSubclassOf<UGameplayEffect> GetBleedingEffectClass()    const override { return BleedingEffectClass; }

	UFUNCTION(BlueprintCallable, Category = "PD|Weapon")
	void AttachActorToWeaponSocket(AActor* ActorToAttach);

	UFUNCTION(BlueprintPure, Category = "PD|Weapon")
	virtual APDWeaponBase* GetCurrentWeapon() const { return nullptr; }

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Damage")
	void OnDeath(AActor* Killer);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Damage")
	void OnDowned(AActor* InstigatorActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Damage")
	void OnRevived(AActor* Reviver);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Damage")
	void OnDamageFeedback(float DamageAmount, FVector HitLocation, AActor* InstigatorActor);

	UFUNCTION(BlueprintPure, Category = "PD|Damage")
	FORCEINLINE EPDLifeState GetLifeState() const { return LifeState; }

	UFUNCTION(BlueprintPure, Category = "PD|Damage")
	FORCEINLINE bool IsDowned() const { return LifeState == EPDLifeState::Downed; }

	UFUNCTION(BlueprintPure, Category = "PD|Damage")
	FORCEINLINE bool IsGettingUp() const { return LifeState == EPDLifeState::GettingUp; }

	UFUNCTION(BlueprintPure, Category = "PD|Damage")
	FORCEINLINE bool IsDead() const { return LifeState == EPDLifeState::Dead; }

	UFUNCTION(BlueprintPure, Category = "PD|Damage")
	float GetDownedRemainingTime() const;

	UFUNCTION(BlueprintPure, Category = "PD|Damage")
	FORCEINLINE float GetDownedLifeSpan() const { return DownedLifeSpan; }

	UFUNCTION(BlueprintPure, Category = "PD|Revive")
	FORCEINLINE float GetReviveTime() const { return ReviveTime; }

	UFUNCTION(BlueprintPure, Category = "PD|Revive")
	float GetReviveRemainingTime() const;

	UFUNCTION(BlueprintPure, Category = "PD|Revive")
	float GetReviveProgress() const;

	UFUNCTION(BlueprintPure, Category = "PD|Revive")
	FORCEINLINE bool IsBeingRevived() const { return ActiveReviver.Get() != nullptr; }

	UFUNCTION(BlueprintPure, Category = "PD|Revive")
	FORCEINLINE AActor* GetActiveReviver() const { return ActiveReviver.Get(); }

	UFUNCTION(BlueprintCallable, Category = "PD|Revive")
	bool BeginReviveInteraction(AActor* Reviver);

	UFUNCTION(BlueprintCallable, Category = "PD|Revive")
	void CancelReviveInteraction(AActor* Reviver);

	UFUNCTION(BlueprintCallable, Category = "PD|Revive")
	void FinishGettingUp();

	virtual void HandleDeath(AActor* Killer);
	virtual void HandleDowned(AActor* InstigatorActor);
	virtual void Revive(AActor* Reviver);
	virtual bool ShouldEnterDownedStateOnLethalDamage() const { return false; }
	bool CanBeKilledWhileDownedByDamage() const;

	UPROPERTY(BlueprintReadOnly, Category="PD|Damage")
	bool bIsDead = false;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_LifeState, BlueprintReadOnly, Category="PD|Damage")
	EPDLifeState LifeState = EPDLifeState::Alive;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="PD|Damage")
	float DownedExpireServerTime = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="PD|Revive")
	TObjectPtr<AActor> ActiveReviver;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="PD|Revive")
	float ReviveEndServerTime = 0.f;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY()
	TObjectPtr<UPDAttributeSet> AttributeSet;

	virtual void InitAbilitySystem();
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void OnLifeStateChanged(EPDLifeState OldLifeState, AActor* ContextActor);
	void ResetLifeStateToAlive(AActor* ContextActor);

	UFUNCTION()
	void OnRep_LifeState(EPDLifeState OldLifeState);

private:
	void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);
	void SetLifeState(EPDLifeState NewLifeState, AActor* ContextActor);
	void SyncLifeStateTags();
	void ApplyLifeStateMovement();
	void RestoreAliveCollisionAndInput();
	void ApplyReviveHealth(AActor* Reviver);
	void StartDownedDeathTimer();
	void ClearDownedDeathTimer();
	void HandleDownedExpired();
	void CompleteReviveInteraction();
	void TickReviveInteraction();
	void ClearReviveInteraction(bool bCancelled);
	bool IsValidReviver(AActor* Reviver) const;

	bool bAbilitySystemDelegatesBound = false;
	bool bAttributesInitialized = false;
	bool bStartupAbilitiesGiven = false;
	bool bActiveAbilitiesGiven = false;

	FTimerHandle DownedDeathTimerHandle;
	FTimerHandle ReviveTimerHandle;
	FVector ReviveStartLocation = FVector::ZeroVector;
	float DownedEnteredServerTime = 0.f;
	TWeakObjectPtr<AActor> PendingGetUpContext;
};
