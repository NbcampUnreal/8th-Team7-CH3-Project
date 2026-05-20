#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Enemy/Types/EnemyTypes.h"
#include "Enemy/Interfaces/PDCombatInterface.h"
#include "Interfaces/PDDetectable.h"
#include "PDEnemyBase.generated.h"

class APDItemBase;
class APDWeaponBase;
class UWidgetComponent;
class UPDEnemyOverheadWidget;
class USoundBase;
class USoundAttenuation;
struct FOnAttributeChangeData;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDEnemyBase : public APDCharacterBase, public IPDCombatInterface, public IPDDetectable
{
	GENERATED_BODY()

public:
	APDEnemyBase();

	//~ Begin IPDCombatInterface
	virtual uint8 GetTeamID_Implementation() const override;
	virtual EPDStaminaStatus GetStaminaStatus_Implementation() const override;
	//~ End IPDCombatInterface

	// Combat이 아닐 때 피격되면 공격자 위치를 NoiseHint로 등록해 BT가 그 방향으로 추적.
	virtual void ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo) override;

	UFUNCTION(BlueprintCallable, Category = "PD|AI")
	void SetEnemyState(EPDEnemyState NewState);

	UFUNCTION(BlueprintPure, Category = "PD|AI")
	FORCEINLINE EPDEnemyState GetEnemyState() const { return CurrentState; }

	// 조준 타겟 설정 (BT Task에서 SetAimTarget 호출)
	UFUNCTION(BlueprintCallable, Category = "PD|AI|Weapon")
	void SetAimTarget(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "PD|AI|Weapon")
	void ClearAimTarget();

	// 사수 개인별 조준 편향(deg). 무기 GetAimDirectionFromOwner 가 spread 전에 회전 적용 →
	// Pitch=위아래, Yaw=좌우 — 인스턴스마다 탄착군이 다른 위치에 형성.
	UFUNCTION(BlueprintPure, Category = "PD|AI|Aim")
	FORCEINLINE FRotator GetAimBias() const { return AimBias; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// 이동 누적 거리로 발자국 트리거 — ABP/AnimNotify 미설정 적도 발소리/AI 노이즈 보장.
	void TickFootstep(float DeltaSeconds);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|AI")
	EPDEnemyState CurrentState = EPDEnemyState::Idle;

	/** Pitch 중심 오프셋(deg). 무기 forward 가 플레이어 머리로 정렬되는 보정용 — 음수=아래로 내려 몸통 기준. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|AI|Aim")
	float AimBiasPitchOffset = -5.0f;

	/** 위아래(Pitch) 편향 최대(deg). 사람 손은 보통 상하 흔들림이 좌우보다 큼. BeginPlay 에서 [Offset-Max..Offset+Max] 균등분포로 한 번 굴림. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|AI|Aim", meta = (ClampMin = "0.0"))
	float AimBiasMaxPitchDegrees = 7.0f;

	/** 좌우(Yaw) 편향 최대(deg). BeginPlay 에서 [-Max..+Max] 균등분포로 한 번 굴림. 0=좌우 정확. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|AI|Aim", meta = (ClampMin = "0.0"))
	float AimBiasMaxYawDegrees = 3.0f;

	/** 본 인스턴스 고정 Bias. BeginPlay 에서 한 번 굴린 뒤 변하지 않음 — 사수의 "버릇". */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "PD|AI|Aim")
	FRotator AimBias = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Quest")
	FName QuestEnemyID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Quest")
	bool bIsQuestEnemy = false;

	/** 디자이너 확장 hook. 상태 전환 직후 호출. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|AI")
	void OnEnemyStateChanged(EPDEnemyState NewState);

	/** 상태별 진입 hook. 자식 클래스에서 오버라이드 가능. */
	virtual void OnEnterState_Idle()   {}
	virtual void OnEnterState_Alert()  {}
	virtual void OnEnterState_Chase()  {}
	virtual void OnEnterState_Combat() {}
	virtual void OnEnterState_Dead();

	virtual void HandleDeath(AActor* Killer) override;
	virtual void OnVisionExposureChanged_Implementation(AActor* Observer, float Exposure) override;

	// ─── 사망 드랍 ─────────────────────────────────────────────
	/** 사망 시 굴릴 드랍 테이블. 디자이너 BP 디폴트에서 채움. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot")
	TArray<FPDLootEntry> LootTable;

	/** 시체/박스 등 상호작용 가능한 컨테이너. 비어있으면 시체 미스폰. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot")
	TSubclassOf<AActor> CorpseContainerClass;

	/** 드랍 위치 변동 반경 — 0 이면 정확히 사망 위치에 스폰. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot", meta = (ClampMin = "0.0"))
	float LootSpawnRadius = 50.f;

	/** 적이 들고 있던 무기의 시체 Stash 이전 확률. 0=절대 안 떨어뜨림, 1=항상 보장. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Loot|Weapon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WeaponDropChance = 0.8f;

	/** LootTable을 굴려 아이템을 스폰. 호출 시점은 자식이 결정 가능 (기본은 OnEnterState_Dead). */
	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	virtual void DropLootOnDeath();

	/** 시체/박스 컨테이너 스폰. 비어있는 클래스면 nullptr. 결과는 CorpseContainerInstance 로도 보관. */
	UFUNCTION(BlueprintCallable, Category = "PD|Loot")
	virtual AActor* SpawnCorpseContainer();

	UFUNCTION(BlueprintPure, Category = "PD|Loot")
	FORCEINLINE AActor* GetCorpseContainer() const { return CorpseContainerInstance.Get(); }

	// ─── Equipped Weapon Drop ─────────────────────────────────────────────
	/**
	 * 사망 시 시체 컨테이너에 추가될 장착 무기의 ItemID. 자식이 override —
	 * 무기 미장착이거나 비-드랍성 무기면 NAME_None 반환.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "PD|Loot|Weapon")
	FName GetEquippedWeaponItemID() const;
	virtual FName GetEquippedWeaponItemID_Implementation() const { return NAME_None; }

	/** WeaponDropChance 굴려 성공 시 GetEquippedWeaponItemID 를 시체 Stash 에 1개 추가. */
	UFUNCTION(BlueprintCallable, Category = "PD|Loot|Weapon")
	virtual void TryDropEquippedWeaponToCorpse();

	/** 사망 시 스폰된 컨테이너 — 자식 클래스가 장착무기 등 추가 아이템 이전 시 참조. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "PD|Loot")
	TWeakObjectPtr<AActor> CorpseContainerInstance;

	/** 디자이너가 BP에서 드랍 후 처리(VFX/사운드 등) 작성 가능. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Loot")
	void OnLootDropped(const TArray<AActor*>& SpawnedItems);

	/** 사망 후 본체(메시) 소멸까지 대기 시간(초). 0 이하면 LifeSpan 미설정(영구). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|AI|Death", meta = (ClampMin = "0.0"))
	float CorpseDespawnDelay = 1.f;

	// ─── 오버헤드 HUD (HP Bar + 상태 말풍선) ─────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|HUD", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> OverheadWidgetComponent;

	/** BP 에서 지정한 위젯 클래스. 미지정 시 헤드 HUD 미생성. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|HUD")
	TSubclassOf<UPDEnemyOverheadWidget> OverheadWidgetClass;

	/** 상태 전환 표시 시간(초). 0 이하면 수동 숨김까지 유지. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|HUD", meta = (ClampMin = "0.0"))
	float SpeechBubbleDuration = 1.f;

	/** 사망 애님 몽타그 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	// ─── 사운드 (Spatialized) ──────────────────────────────────
	/** 모든 enemy 사운드에 공유되는 거리감쇄/공간화 설정. 미지정 시 2D 재생. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Audio")
	TObjectPtr<USoundAttenuation> SoundAttenuation;

	/** 사망 시 1회 재생. 액터 소멸과 무관하게 끝까지 재생됨. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Audio")
	TObjectPtr<USoundBase> DeathSound;

	/** 피격 시 재생 (살아있는 동안만 — 사망 후 데미지는 무시). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Audio")
	TObjectPtr<USoundBase> HurtSound;

	/** 이동 중 일정 간격으로 재생. 미지정 시 발소리 비활성. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Audio")
	TObjectPtr<USoundBase> FootstepSound;

	/** 한 발자국당 누적 이동 거리(cm). 작을수록 발자국이 잦음. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Audio", meta = (ClampMin = "10.0"))
	float FootstepStrideDistance = 250.f;

	/** 이 속도(cm/s) 미만이면 발소리 미생성 — 미세 슬라이드/정지 잡소리 방지. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Audio", meta = (ClampMin = "0.0"))
	float FootstepMinSpeed = 50.f;

	/** 발자국으로 발생시킬 AI 청각 노이즈 반경(cm). 0 이면 노이즈 미발생. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Audio", meta = (ClampMin = "0.0"))
	float FootstepNoiseRange = 800.f;

private:
	// 발자국 트리거용 이동 거리 누적.
	float FootstepDistanceAccumulator = 0.f;

	UFUNCTION()
	void OnCombatTargetChanged(AActor* NewTarget);

	void OnTorsoHPChanged(const FOnAttributeChangeData& Data);

	UPROPERTY()
	TObjectPtr<UPDEnemyOverheadWidget> OverheadWidget;

	bool bHealthBarShown = false;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DitherMaterials;
};
