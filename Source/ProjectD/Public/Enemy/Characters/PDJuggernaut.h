#pragma once

#include "CoreMinimal.h"
#include "Enemy/Characters/PDSoldier.h"
#include "PDJuggernaut.generated.h"

class USkeletalMeshComponent;
class APDEnemyAIControllerBase;
class APDProjectile;
class APDJuggernautMissile;
class UAnimSequence;
class UParticleSystem;
class UAudioComponent;
class USoundBase;

/** Juggernaut 보스 생애주기 단계. BB(BossPhase)로 노출 → BT 분기 게이트. */
UENUM(BlueprintType)
enum class EPDJuggernautPhase : uint8
{
	Dormant      UMETA(DisplayName = "Dormant"),       // 비활성: 원위치 대기
	Active       UMETA(DisplayName = "Active"),         // 활성: Strafe + Sniper + 패턴
	Deactivating UMETA(DisplayName = "Deactivating"),   // 범위 이탈: 정지 후 카운트다운
	Returning    UMETA(DisplayName = "Returning"),      // 풀회복 후 원위치 복귀
};

/** 공격 패턴 식별자. 패턴2·3(미사일)은 후속 단계에서 구현. */
UENUM(BlueprintType)
enum class EPDJuggernautPattern : uint8
{
	None       UMETA(DisplayName = "None"),
	Pulverize  UMETA(DisplayName = "Pulverize (MachineGun)"),    // 패턴1 분쇄
	Annihilate UMETA(DisplayName = "Annihilate (Missile Swarm)"),// 패턴2 섬멸
};

/**
 * Juggernaut 보스.
 *  - APDSoldier 상속: Sniper 일반공격(단발 경로)·LootBox 드랍·부위별 HP 를 그대로 재사용.
 *    → 디자이너 BP: DefaultWeaponClass=Sniper, CombatComponent.AttackRange=2500, LootTable/CorpseContainerClass 설정.
 *  - 본 클래스가 추가하는 것: 활성화 생애주기 + 랜덤 쿨다운 패턴 + 거치무기.
 *
 * 역할 분담:
 *  - C++  : 활성/비활성/풀회복/복귀 상태머신, 패턴 디스패처·타이밍·데미지. 타겟은 CombatComponent 로 지정
 *           (RequestAttack 검증 + AIController 가 BB.TargetActor 로 자동 동기화).
 *  - BT   : BossPhase / bIsExecutingPattern / TargetActor 를 읽어 Strafe·FireAtTarget(Sniper)·MoveTo(Home) 분기.
 *  - BP   : AoE 인디케이터·머즐 VFX·몽타주 등 연출 (BP_On* 훅).
 *
 * 활성화 규칙(요구사항):
 *  - ActivationRange(2500) 안에 플레이어 진입 → 활성(타겟 지정).
 *  - 범위 이탈 → 그 자리 정지 후 DeactivationDelay(5초) → 비활성(타겟 해제) + 풀회복 + 원위치 복귀.
 *  - 카운트다운 중 재진입 → 즉시 재활성.
 *  - 패턴 시작 후에는 거리 조건을 벗어나도 중단하지 않음(비활성화 보류).
 */
UCLASS(Blueprintable)
class PROJECTD_API APDJuggernaut : public APDSoldier
{
	GENERATED_BODY()

public:
	APDJuggernaut();

	UFUNCTION(BlueprintPure, Category = "PD|Boss")
	FORCEINLINE EPDJuggernautPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "PD|Boss")
	FORCEINLINE bool IsBossActive() const { return Phase == EPDJuggernautPhase::Active; }

	UFUNCTION(BlueprintPure, Category = "PD|Boss")
	FORCEINLINE bool IsExecutingPattern() const { return bIsExecutingPattern; }

	/** 패턴2 런처 애님의 발사 프레임 노티파이가 호출 — 미사일 스웜 발사 시퀀스 시작(서버 전용). */
	void OnPattern2LaunchNotify();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnEnterState_Dead() override;

	// ─── 거치 무기 (메시만 부착, 발사·타이밍은 본 클래스가 직접 제어) ─────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Boss|Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> MachineGunMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Boss|Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> MissileLauncherMesh;

	/** 본체 메시에서 머신건 메시를 부착할 소켓. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Weapon")
	FName MachineGunSocketName = TEXT("MachineGunSocket");

	/** 본체 메시에서 미사일포 메시를 부착할 소켓. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Weapon")
	FName MissilePortSocketName = TEXT("MissilePortSocket");

	// ─── 활성화 ───────────────────────────────────────────────────────────────
	/** 이 거리 안에 플레이어가 들어오면 활성화. 이탈 판정도 동일 거리. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Activation", meta = (ClampMin = "0.0"))
	float ActivationRange = 2500.f;

	/** 범위 이탈 후 비활성화까지 정지 대기 시간(초). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Activation", meta = (ClampMin = "0.0"))
	float DeactivationDelay = 5.f;

	/** 활성화 범위 판정 주기(초). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Activation", meta = (ClampMin = "0.05"))
	float ActivationCheckInterval = 0.2f;

	/** 원위치 도착 판정 허용 반경(cm). 이 안에 들어오면 Dormant 로 전환. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Activation", meta = (ClampMin = "0.0"))
	float HomeArrivalTolerance = 250.f;

	// ─── 패턴 스케줄 ──────────────────────────────────────────────────────────
	/** 다음 패턴까지 쿨다운 랜덤 범위(초) 하한. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern", meta = (ClampMin = "0.0"))
	float PatternCooldownMin = 6.f;

	/** 다음 패턴까지 쿨다운 랜덤 범위(초) 상한. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern", meta = (ClampMin = "0.0"))
	float PatternCooldownMax = 10.f;

	// ─── 패턴1: 분쇄 (머신건) ─────────────────────────────────────────────────
	/** 패턴1 발동 조건: 타겟 거리 ≤ 이 값. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1", meta = (ClampMin = "0.0"))
	float Pattern1MaxRange = 1200.f;

	/** 조준·차징 시간(초). 이 동안 인디케이터가 플레이어를 추적(연출은 BP). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1", meta = (ClampMin = "0.0"))
	float Pattern1ChargeTime = 5.f;

	/** 발사 지속 시간(초). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1", meta = (ClampMin = "0.0"))
	float Pattern1FireDuration = 4.f;

	/** 발사 틱 간격(초) — 총알 발사·머즐플래시 빈도(작을수록 연사). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1", meta = (ClampMin = "0.02"))
	float Pattern1FireInterval = 0.04f;

	/** 발사 사거리(cm). 활성/비활성 거리(2500)와 동일 — 패턴 시작 후 거리 무관 지속. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1", meta = (ClampMin = "0.0"))
	float Pattern1Range = 2500.f;

	/** AoE 반각(deg). 잠긴 조준 방향 기준 좌우 허용각 — 이 부채꼴 안의 플레이어만 피격. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float Pattern1ConeHalfAngleDeg = 12.f;

	/** 발사 틱당 데미지. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1", meta = (ClampMin = "0.0"))
	float Pattern1DamagePerTick = 6.f;

	/** 머신건 머즐 소켓들(MachineGunMesh 기준). 발사 틱마다 번갈아 사용. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1")
	TArray<FName> Pattern1MuzzleSockets = { TEXT("MGMuzzle1"), TEXT("MGMuzzle2") };

	/** 발사 틱마다 머즐에서 스폰할 총알 발사체. 미설정 시 발사체 없음(연출만). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1")
	TSubclassOf<APDProjectile> MachineGunProjectileClass;

	/** 발사 중 MachineGunMesh 에서 루프 재생할 발사 애니메이션(배럴 회전·반동 등). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1")
	TObjectPtr<UAnimSequence> MachineGunFireAnim;

	/** 발사 틱마다 머즐 소켓에 부착 스폰할 머즐 플래시 파티클(라이플의 MuzzleFlashEffect 에셋 그대로 사용 가능). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1")
	TObjectPtr<UParticleSystem> MuzzleFlashFX;

	/** 차징(조준) 시작 시 재생할 사운드 — 스핀업. 발사 시작 시 자동 정지. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1")
	TObjectPtr<USoundBase> Pattern1ChargeSound;

	/** 발사 동안 루프 재생할 사운드 — 연사. 루프 가능한 에셋 권장(발사 종료 시 정지). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1")
	TObjectPtr<USoundBase> Pattern1FireLoopSound;

	/** 조준·차징 단계 및 패턴2 회전 속도(deg/s). 빠르게 플레이어를 향함. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern", meta = (ClampMin = "1.0"))
	float PatternTurnRateDegPerSec = 180.f;

	/** 패턴1 발사 중 회전 속도(deg/s). 느리게 추적 — 걷는 플레이어는 추적, 달리는 플레이어는 콘에서 이탈. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern1", meta = (ClampMin = "0.0"))
	float Pattern1FireTurnRateDegPerSec = 30.f;

	// ─── 패턴2: 섬멸 (미사일 스웜) ────────────────────────────────────────────
	/** 발사할 미사일 수. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2", meta = (ClampMin = "1"))
	int32 Pattern2MissileCount = 8;

	/** 발사 해치 소켓들(MissileLauncherMesh 기준). 발사마다 순환 사용. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2")
	TArray<FName> Pattern2HatchSockets = {
		TEXT("Missilehatch0"), TEXT("Missilehatch1"), TEXT("Missilehatch2"), TEXT("Missilehatch3"),
		TEXT("Missilehatch4"), TEXT("Missilehatch5"), TEXT("Missilehatch6"), TEXT("Missilehatch7") };

	/** 볼리 간 발사 간격(초) — 볼리 단위 순차 stagger. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2", meta = (ClampMin = "0.0"))
	float Pattern2LaunchInterval = 0.15f;

	/** 볼리당 동시 발사 미사일 수. 해치를 균등 분할해 짝지어 발사(예: 해치 8·2발 → 0&4,1&5,2&6,3&7). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2", meta = (ClampMin = "1"))
	int32 Pattern2MissilesPerVolley = 2;

	/** 발사 런처 애님(원샷: 발사각으로 raise). 14프레임 등에 PDJuggernautLaunchNotify 를 배치해 발사 시작. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2")
	TObjectPtr<UAnimSequence> Pattern2LauncherAnim;

	/** 발사 시 스폰할 코스메틱 미사일(포물선 비행 후 폭발). 미설정 시 미사일 비주얼 없음(데미지는 그대로 적용). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2")
	TSubclassOf<APDJuggernautMissile> Pattern2MissileClass;

	/** 미사일 발사 시 재생할 사운드(발사마다 1회, 해치 위치). 미설정 시 발사음 없음. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2")
	TObjectPtr<USoundBase> Pattern2LaunchSound;

	/** 발사 시작 안전망(초). 노티파이가 이 시간 내 발사를 못 시작하면 강제 발사(소프트락 방지). 애님 발사프레임 시간보다 길게. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2", meta = (ClampMin = "0.1"))
	float Pattern2LaunchFallbackDelay = 2.0f;

	/** 발사 → 착탄 시간(초). 인디케이터 차오름 시간 = 미사일 비행 시간(BP 연출과 동기). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2", meta = (ClampMin = "0.1"))
	float Pattern2TravelTime = 2.5f;

	/** 착탄 지점이 플레이어 주변으로 흩어지는 반경(cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2", meta = (ClampMin = "0.0"))
	float Pattern2ScatterRadius = 800.f;

	/** 착탄 지점 간 최소 간격(cm). 스웜 AoE 가 한곳에 뭉쳐 겹치지 않도록 분산(공간이 좁으면 가능한 최대로). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2", meta = (ClampMin = "0.0"))
	float Pattern2MinSeparation = 400.f;

	/** 착탄 AoE 반경(cm). 이 안의 플레이어가 피해. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2", meta = (ClampMin = "0.0"))
	float Pattern2ImpactRadius = 250.f;

	/** 착탄 1회당 (부위당) 기본 데미지. Pattern2BoneDamageOverrides 에 없는 본은 이 값. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2", meta = (ClampMin = "0.0"))
	float Pattern2Damage = 40.f;

	/** 폭발 시 데미지를 줄 부위 본 이름들 — 부위마다 1개씩 지정 시 각 부위가 Pattern2Damage 만큼 피해.
	 *  대상 BodyPartConfig(DA_BodyPartConfig)에 등록된 이름이어야 해당 부위로 라우팅됨. 비우면 단일 부위(Torso).
	 *  기본값 = DA_BodyPartConfig 등록 본(Head/Torso/Arm_L/Arm_R/Leg_L/Leg_R). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2")
	TArray<FName> Pattern2ImpactBones = {
		TEXT("neck_01"), TEXT("spine_01"),
		TEXT("upperarm_l"), TEXT("upperarm_r"),
		TEXT("thigh_l"), TEXT("thigh_r") };

	/** 부위별 데미지 오버라이드(본 이름 → 데미지). 등록된 본은 Pattern2Damage 대신 이 값.
	 *  키는 Pattern2ImpactBones 의 본과 일치해야 적용됨. 기본: 머리(neck_01)=20, 나머지=Pattern2Damage(40). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Boss|Pattern2")
	TMap<FName, float> Pattern2BoneDamageOverrides = { { TEXT("neck_01"), 20.f } };

	// ─── BP 연출 훅 (타이밍·데미지는 C++, 비주얼만 BP) ──────────────────────────
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss")
	void BP_OnActivated(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss")
	void BP_OnBeginDeactivation();

	/** 비활성화 확정(풀회복 직후). 복귀 시작 시점. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss")
	void BP_OnDeactivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss")
	void BP_OnReturnHomeComplete();

	/** 패턴1 조준 시작 — 빨간 AoE 인디케이터 표시 + 차징(ChargeTime). Target 을 향해 표시. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss|Pattern1")
	void BP_OnPattern1BeginAim(AActor* Target);

	/** 패턴1 발사 시작 — 조준 방향 잠금. 머신건 루프 VFX/사운드 시작, 인디케이터 고정. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss|Pattern1")
	void BP_OnPattern1BeginFire(FVector AimDirection);

	/** 발사 틱마다 — 머즐 플래시/트레이서 등. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss|Pattern1")
	void BP_OnPattern1FireShot(FName MuzzleSocket, FVector MuzzleLocation);

	/** 패턴1 종료 — 인디케이터 숨김, 머신건 VFX 정지. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss|Pattern1")
	void BP_OnPattern1End();

	/** 패턴2 시작(스웜 발사 직전 windup 연출용). */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss|Pattern2")
	void BP_OnPattern2Begin(AActor* Target);

	/** 미사일 1발 발사 — 해치에서 cosmetic 미사일 스폰 + 착탄 지점에 인디케이터(TravelTime 동안 차오름). */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss|Pattern2")
	void BP_OnPattern2Launch(FName HatchSocket, FVector LaunchLocation, FVector ImpactLocation, float TravelTime);

	/** 착탄 — 폭발 VFX/사운드. 데미지는 C++가 적용. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss|Pattern2")
	void BP_OnPattern2Impact(FVector ImpactLocation);

	/** 패턴2 종료(모든 미사일 착탄 완료). */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Boss|Pattern2")
	void BP_OnPattern2End();

private:
	// ─── 연출 멀티캐스트 (서버 → 전 클라) ──────────────────────────────────────
	// 패턴 상태머신·타이밍은 서버 전용이므로 BP_On* 훅도 서버에서만 발화 → 클라 VFX 미동기화.
	// 서버 로직은 BP_On* 를 직접 호출하지 않고 아래 Multicast_* 를 호출 → 모든 머신(리슨 호스트 포함)에서 BP 훅 발화.
	// 고빈도 틱 연출(머즐/착탄)은 Unreliable, 1회성 상태 전환은 Reliable.
	UFUNCTION(NetMulticast, Reliable)   void Multicast_OnActivated(AActor* Target);
	UFUNCTION(NetMulticast, Reliable)   void Multicast_OnBeginDeactivation();
	UFUNCTION(NetMulticast, Reliable)   void Multicast_OnDeactivated();
	UFUNCTION(NetMulticast, Reliable)   void Multicast_OnReturnHomeComplete();
	UFUNCTION(NetMulticast, Reliable)   void Multicast_OnPattern1BeginAim(AActor* Target);
	UFUNCTION(NetMulticast, Reliable)   void Multicast_OnPattern1BeginFire(FVector AimDirection);
	UFUNCTION(NetMulticast, Unreliable) void Multicast_OnPattern1FireShot(FName MuzzleSocket, FVector MuzzleLocation);
	UFUNCTION(NetMulticast, Reliable)   void Multicast_OnPattern1End();
	UFUNCTION(NetMulticast, Reliable)   void Multicast_OnPattern2Begin(AActor* Target);
	UFUNCTION(NetMulticast, Reliable)   void Multicast_OnPattern2Launch(FName HatchSocket, FVector LaunchLocation, FVector ImpactLocation, float TravelTime);
	UFUNCTION(NetMulticast, Reliable)   void Multicast_OnPattern2Impact(FVector ImpactLocation);
	UFUNCTION(NetMulticast, Reliable)   void Multicast_OnPattern2End();

	// ─── 활성화 생애주기 ───
	void TickActivationCheck();
	void EnterPhase(EPDJuggernautPhase NewPhase);
	void Activate(AActor* Target);
	void BeginDeactivation();
	void CompleteDeactivation();      // 풀회복 + Returning 전환
	void CheckReturnHomeArrival();
	AActor* FindNearestPlayerInRange(float Range) const;
	void FullHeal();                  // 전 부위 HP = Max

	// ─── 패턴 ───
	void ScheduleNextPattern();
	void TryStartPattern();
	EPDJuggernautPattern SelectPatternByDistance(float DistToTarget) const;
	void StartPattern1(AActor* Target);
	void Pattern1BeginFire();
	void Pattern1FireTick();
	void EndPattern();
	void SpawnMachineGunProjectile(const FVector& MuzzleLoc);   // 총구 방향으로 총알 발사(명중 시 데미지)
	void StopPattern1Audio();                                   // 차징/발사 루프 사운드 정지

	void StartPattern2(AActor* Target);
	void Pattern2LaunchVolley();                  // 볼리(짝지은 해치) 동시 발사 + 볼리 종료 판정
	void Pattern2LaunchMissile(int32 HatchIndex); // 지정 해치에서 미사일 1발 발사 + 착탄 예약
	void TickPattern2Impacts();         // world-time 기준 착탄 처리(Tick에서 호출)
	void ApplyPattern2Damage(const FVector& Center);
	FVector PickScatterImpactLocation(const FVector& Center) const; // 기존 착탄과 떨어지도록 best-candidate 분산
	FVector SnapImpactToGround(const FVector& In) const;            // 착탄 Z를 실제 바닥으로 스냅(데미지는 2D라 무관, 연출 위치 정상화)

	// 패턴 중 플레이어를 향해 yaw 회전(BT/focus 무관, 매 프레임).
	void FacePatternTarget(float DeltaSeconds);

	// pd.ai.debugdraw 1 일 때 범위·상태·패턴·타겟 시각화 + 화면 텍스트(컨트롤러와 동일 토글 공유).
	void DrawBossDebug() const;

	// ─── 타겟/BB 동기화 ───
	APDEnemyAIControllerBase* GetEnemyController() const;
	void SetCombatTarget(AActor* Target);   // CombatComponent 경유 → RequestAttack + BB 자동 동기화
	void SyncStateToBlackboard();           // BossPhase + bIsExecutingPattern

	EPDJuggernautPhase Phase = EPDJuggernautPhase::Dormant;
	bool bIsExecutingPattern = false;
	EPDJuggernautPattern CurrentPattern = EPDJuggernautPattern::None;

	TWeakObjectPtr<AActor> ActiveTarget;

	// 패턴1 발사 단계에서 잠긴 조준 방향(월드, 정규화 2D) + 누적 시간 + 머즐 토글 인덱스.
	FVector Pattern1AimDir = FVector::ForwardVector;
	float   Pattern1FireElapsed = 0.f;
	int32   Pattern1MuzzleIndex = 0;

	// 패턴1 발사 단계 여부 — true 면 FacePatternTarget 가 느린 속도로 회전하고 콘이 그 방향을 따라감.
	bool    bPattern1Firing = false;

	// 패턴1 사운드 핸들(루프 정지 제어용). 연출 멀티캐스트에서 머신마다 로컬 스폰.
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Pattern1ChargeAudio;
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Pattern1FireAudio;

	// 패턴2: 예약된 착탄(위치 + world-time). Tick 이 시간 도달분을 폭발 처리.
	struct FPDPendingImpact
	{
		FVector Location = FVector::ZeroVector;
		float   ImpactTime = 0.f;
	};
	TArray<FPDPendingImpact> Pattern2PendingImpacts;
	int32 Pattern2LaunchedCount = 0;
	int32 Pattern2VolleyIndex = 0;

	FTimerHandle ActivationTimerHandle;
	FTimerHandle DeactivationTimerHandle;
	FTimerHandle PatternCooldownHandle;
	FTimerHandle PatternChargeHandle;    // 조준 → 발사 전환
	FTimerHandle Pattern1FireHandle;     // 발사 루프
	FTimerHandle Pattern2LaunchHandle;   // 미사일 스웜 stagger 발사
	FTimerHandle Pattern2LaunchFallbackHandle; // 노티파이 미발화 시 발사 강제 시작(안전망)
};
