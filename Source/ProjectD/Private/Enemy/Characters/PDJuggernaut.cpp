#include "Enemy/Characters/PDJuggernaut.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimSequence.h"
#include "AttributeSet/PDAttributeSet.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/AI/Controllers/PDEnemyAIControllerBase.h"
#include "Enemy/Characters/PDJuggernautMissile.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/PDDamageable.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Weapons/PDProjectile.h"

#if ENABLE_DRAW_DEBUG
// 컨트롤러가 정의한 pd.ai.debugdraw 토글을 보스도 공유 — 처음 호출 시 한 번 찾아 캐시.
static IConsoleVariable* GetJuggernautDebugCVar()
{
	static IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("pd.ai.debugdraw"));
	return CVar;
}
#endif

APDJuggernaut::APDJuggernaut()
{
	TeamID = 2; // Hostile.

	// 거치 무기는 메시만 부착 — 발사/타이밍/데미지는 본 클래스가 직접 제어(무기 프레임워크 미사용).
	MachineGunMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MachineGunMesh"));
	MachineGunMesh->SetupAttachment(GetMesh(), MachineGunSocketName);
	MachineGunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MissileLauncherMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MissileLauncherMesh"));
	MissileLauncherMesh->SetupAttachment(GetMesh(), MissilePortSocketName);
	MissileLauncherMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APDJuggernaut::BeginPlay()
{
	Super::BeginPlay();

	// 디자이너가 소켓 이름을 변경했을 수 있으니 현재 값으로 재부착(ctor 부착은 C++ 디폴트 기준).
	if (USkeletalMeshComponent* Body = GetMesh())
	{
		const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
		if (MachineGunMesh)      MachineGunMesh->AttachToComponent(Body, Rules, MachineGunSocketName);
		if (MissileLauncherMesh) MissileLauncherMesh->AttachToComponent(Body, Rules, MissilePortSocketName);
	}

	// 활성화 범위 판정은 서버에서만. 폰 BeginPlay 시점에 컨트롤러가 아직 없을 수 있으므로
	// 루프 타이머로 돌리고 매 tick 컨트롤러 유효성을 확인.
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(ActivationTimerHandle, this,
			&APDJuggernaut::TickActivationCheck, ActivationCheckInterval, /*bLoop=*/true);
	}
}

void APDJuggernaut::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 패턴(조준+발사) 동안 플레이어를 지속 추적 — BT/focus 와 무관하게 C++가 직접 yaw 회전.
	if (bIsExecutingPattern)
	{
		FacePatternTarget(DeltaSeconds);

		// 패턴2: 예약된 미사일 착탄을 world-time 으로 처리.
		if (CurrentPattern == EPDJuggernautPattern::Annihilate)
		{
			TickPattern2Impacts();
		}
	}

#if ENABLE_DRAW_DEBUG
	if (const IConsoleVariable* CVar = GetJuggernautDebugCVar())
	{
		if (CVar->GetInt() != 0)
		{
			DrawBossDebug();
		}
	}
#endif
}

void APDJuggernaut::FacePatternTarget(float DeltaSeconds)
{
	const AActor* Target = ActiveTarget.Get();
	if (!Target) return;

	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero()) return;

	// 발사 중엔 느린 회전(걷기 추적 수준) — 달리는 플레이어는 콘에서 이탈. 조준/차징·패턴2 는 빠르게 추적.
	const float TurnRate = bPattern1Firing ? Pattern1FireTurnRateDegPerSec : PatternTurnRateDegPerSec;

	const FRotator CurrentRot = GetActorRotation();
	const FRotator DesiredRot(CurrentRot.Pitch, ToTarget.Rotation().Yaw, CurrentRot.Roll);
	const FRotator NewRot = FMath::RInterpConstantTo(CurrentRot, DesiredRot, DeltaSeconds, TurnRate);

	// 액터 + ControlRotation 동시 갱신 — CharacterMovement 회전 정책(bUseControllerDesiredRotation 등)과 충돌 방지.
	SetActorRotation(NewRot);
	if (APDEnemyAIControllerBase* AICon = GetEnemyController())
	{
		AICon->SetControlRotation(FRotator(0.f, NewRot.Yaw, 0.f));
	}

	// 발사 중 데미지 콘은 실제 바라보는 방향(천천히 회전)을 따라감 — 회전이 느려 빠른 플레이어는 콘 밖으로.
	if (bPattern1Firing)
	{
		Pattern1AimDir = NewRot.Vector().GetSafeNormal2D();
	}
}

void APDJuggernaut::OnEnterState_Dead()
{
	// 모든 보스 타이머 정리 후 부모 사망 처리(발사 timer 정지·무기·LootBox 드랍).
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();
		TM.ClearTimer(ActivationTimerHandle);
		TM.ClearTimer(DeactivationTimerHandle);
		TM.ClearTimer(PatternCooldownHandle);
		TM.ClearTimer(PatternChargeHandle);
		TM.ClearTimer(Pattern1FireHandle);
		TM.ClearTimer(Pattern2LaunchHandle);
		TM.ClearTimer(Pattern2LaunchFallbackHandle);
	}

	Pattern2PendingImpacts.Reset();
	bIsExecutingPattern = false;
	CurrentPattern = EPDJuggernautPattern::None;
	bPattern1Firing = false;
	StopPattern1Audio();

	Super::OnEnterState_Dead();
}

// ───────────────────────────── 활성화 생애주기 ─────────────────────────────

void APDJuggernaut::TickActivationCheck()
{
	if (!HasAuthority() || IsDead()) return;

	switch (Phase)
	{
	case EPDJuggernautPhase::Dormant:
	{
		if (AActor* Player = FindNearestPlayerInRange(ActivationRange))
		{
			Activate(Player);
		}
		break;
	}
	case EPDJuggernautPhase::Returning:
	{
		// 복귀 중 재진입 시 즉시 재활성, 아니면 원위치 도착 판정.
		if (AActor* Player = FindNearestPlayerInRange(ActivationRange))
		{
			Activate(Player);
		}
		else
		{
			CheckReturnHomeArrival();
		}
		break;
	}
	case EPDJuggernautPhase::Active:
	{
		// 패턴 진행 중에는 거리 무관 — 비활성화 보류(요구사항).
		if (bIsExecutingPattern) break;

		if (AActor* Player = FindNearestPlayerInRange(ActivationRange))
		{
			// 타겟 신선도 유지(가장 가까운 플레이어로 갱신).
			if (Player != ActiveTarget.Get())
			{
				ActiveTarget = Player;
				SetCombatTarget(Player);
			}
		}
		else
		{
			BeginDeactivation();
		}
		break;
	}
	case EPDJuggernautPhase::Deactivating:
	{
		// 카운트다운 중 재진입 → 즉시 재활성(타이머 취소는 Activate 가 수행).
		if (AActor* Player = FindNearestPlayerInRange(ActivationRange))
		{
			Activate(Player);
		}
		break;
	}
	}
}

void APDJuggernaut::EnterPhase(EPDJuggernautPhase NewPhase)
{
	Phase = NewPhase;

	switch (NewPhase)
	{
	case EPDJuggernautPhase::Active:
		SetEnemyState(EPDEnemyState::Combat);
		break;

	case EPDJuggernautPhase::Deactivating:
		// 그 자리에 정지(요구사항). BT 는 BossPhase 로 Strafe/사격 분기를 닫음.
		SetEnemyState(EPDEnemyState::Alert);
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->StopMovementImmediately();
		}
		break;

	case EPDJuggernautPhase::Returning:
	case EPDJuggernautPhase::Dormant:
		SetEnemyState(EPDEnemyState::Idle);
		break;
	}

	SyncStateToBlackboard();
}

void APDJuggernaut::Activate(AActor* Target)
{
	if (!Target) return;

	// 진행 중이던 비활성화 카운트다운 취소.
	GetWorldTimerManager().ClearTimer(DeactivationTimerHandle);

	const bool bWasActive = (Phase == EPDJuggernautPhase::Active);

	ActiveTarget = Target;
	EnterPhase(EPDJuggernautPhase::Active);
	SetCombatTarget(Target);

	// 이미 Active 였으면(타겟 갱신 경로) 패턴 스케줄을 다시 깔지 않음.
	if (!bWasActive)
	{
		ScheduleNextPattern();
		Multicast_OnActivated(Target);
	}
}

void APDJuggernaut::BeginDeactivation()
{
	EnterPhase(EPDJuggernautPhase::Deactivating);

	// 비활성화 중에는 일반공격·패턴 모두 중단.
	StopContinuousFire();
	GetWorldTimerManager().ClearTimer(PatternCooldownHandle);

	Multicast_OnBeginDeactivation();

	GetWorldTimerManager().SetTimer(DeactivationTimerHandle, this,
		&APDJuggernaut::CompleteDeactivation, DeactivationDelay, /*bLoop=*/false);
}

void APDJuggernaut::CompleteDeactivation()
{
	FullHeal();

	ActiveTarget = nullptr;
	SetCombatTarget(nullptr);   // 타겟 해제 → BB.TargetActor 클리어(AIController 동기화)

	EnterPhase(EPDJuggernautPhase::Returning);
	Multicast_OnDeactivated();
	// 실제 원위치 이동은 BT(BossPhase==Returning) 가 MoveTo(HomeLocation) 로 수행.
}

void APDJuggernaut::CheckReturnHomeArrival()
{
	const APDEnemyAIControllerBase* AICon = GetEnemyController();
	const UBlackboardComponent* BB = AICon ? AICon->GetBlackboardComponent() : nullptr;
	if (!BB) return;

	const FVector Home = BB->GetValueAsVector(PDBTKeys::HomeLocation);
	if (Home.ContainsNaN() || !FMath::IsFinite(Home.X)) return;

	if (FVector::DistSquared(GetActorLocation(), Home) <= HomeArrivalTolerance * HomeArrivalTolerance)
	{
		EnterPhase(EPDJuggernautPhase::Dormant);
		Multicast_OnReturnHomeComplete();
	}
}

AActor* APDJuggernaut::FindNearestPlayerInRange(float Range) const
{
	const UWorld* World = GetWorld();
	if (!World) return nullptr;

	const FVector SelfLoc = GetActorLocation();
	const float RangeSq = Range * Range;

	AActor* Closest = nullptr;
	float ClosestSq = TNumericLimits<float>::Max();

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
		if (!PlayerPawn || PlayerPawn == this) continue;

		// 사망/다운된 플레이어는 타겟에서 제외.
		if (PlayerPawn->Implements<UPDDamageable>() && !IPDDamageable::Execute_IsAlive(PlayerPawn)) continue;

		const float DistSq = FVector::DistSquared(SelfLoc, PlayerPawn->GetActorLocation());
		if (DistSq <= RangeSq && DistSq < ClosestSq)
		{
			ClosestSq = DistSq;
			Closest = PlayerPawn;
		}
	}
	return Closest;
}

void APDJuggernaut::FullHeal()
{
	if (!HasAuthority() || !AttributeSet) return;

	// InitializeAttributes 와 동일 패턴 — 전 부위 HP 를 Max 로 복원.
	AttributeSet->SetHeadHP (AttributeSet->GetMaxHeadHP());
	AttributeSet->SetTorsoHP(AttributeSet->GetMaxTorsoHP());
	AttributeSet->SetArmLHP (AttributeSet->GetMaxArmLHP());
	AttributeSet->SetArmRHP (AttributeSet->GetMaxArmRHP());
	AttributeSet->SetLegLHP (AttributeSet->GetMaxLegLHP());
	AttributeSet->SetLegRHP (AttributeSet->GetMaxLegRHP());
}

// ───────────────────────────────── 패턴 ─────────────────────────────────

void APDJuggernaut::ScheduleNextPattern()
{
	if (Phase != EPDJuggernautPhase::Active) return;

	const float Cooldown = FMath::FRandRange(
		FMath::Min(PatternCooldownMin, PatternCooldownMax),
		FMath::Max(PatternCooldownMin, PatternCooldownMax));

	GetWorldTimerManager().SetTimer(PatternCooldownHandle, this,
		&APDJuggernaut::TryStartPattern, FMath::Max(Cooldown, 0.1f), /*bLoop=*/false);
}

void APDJuggernaut::TryStartPattern()
{
	if (Phase != EPDJuggernautPhase::Active || bIsExecutingPattern) return;

	AActor* Target = ActiveTarget.Get();
	if (!Target)
	{
		ScheduleNextPattern();
		return;
	}

	const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	switch (SelectPatternByDistance(Dist))
	{
	case EPDJuggernautPattern::Pulverize:
		StartPattern1(Target);
		break;

	case EPDJuggernautPattern::Annihilate:
		StartPattern2(Target);
		break;

	default:
		// 조건에 맞는 (구현된) 패턴 없음 — 일반공격 유지하며 다음 쿨다운 재시도.
		ScheduleNextPattern();
		break;
	}
}

EPDJuggernautPattern APDJuggernaut::SelectPatternByDistance(float DistToTarget) const
{
	// 거리 1200 이하 → 패턴1(분쇄/머신건), 초과 → 패턴2(섬멸/미사일 스웜).
	if (DistToTarget <= Pattern1MaxRange) return EPDJuggernautPattern::Pulverize;
	return EPDJuggernautPattern::Annihilate;
}

void APDJuggernaut::StartPattern1(AActor* Target)
{
	bIsExecutingPattern = true;
	CurrentPattern = EPDJuggernautPattern::Pulverize;
	bPattern1Firing = false;   // 조준 단계: 빠르게 플레이어를 향해 회전(발사 시작 시 느린 회전으로 전환).
	SyncStateToBlackboard();

	// 패턴 중 일반공격 중단(요구사항). BT 는 bIsExecutingPattern 로 사격/Strafe 분기를 닫음.
	// 조준 단계의 타겟 회전은 Tick(FacePatternTarget)이 담당.
	StopContinuousFire();

	Multicast_OnPattern1BeginAim(Target);

	GetWorldTimerManager().SetTimer(PatternChargeHandle, this,
		&APDJuggernaut::Pattern1BeginFire, FMath::Max(Pattern1ChargeTime, 0.01f), /*bLoop=*/false);
}

void APDJuggernaut::Pattern1BeginFire()
{
	const AActor* Target = ActiveTarget.Get();
	if (!Target)
	{
		EndPattern();
		return;
	}

	// 발사 시작 시점의 조준 방향(이후 FacePatternTarget 이 느린 속도로 이 방향을 갱신).
	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation());
	Pattern1AimDir = ToTarget.GetSafeNormal2D();
	if (Pattern1AimDir.IsNearlyZero()) Pattern1AimDir = GetActorForwardVector().GetSafeNormal2D();

	bPattern1Firing = true;   // 이후 발사 동안 느린 회전 + 총구 방향(Pattern1AimDir) 갱신.

	Pattern1FireElapsed = 0.f;
	Pattern1MuzzleIndex = 0;

	Multicast_OnPattern1BeginFire(Pattern1AimDir);

	GetWorldTimerManager().SetTimer(Pattern1FireHandle, this,
		&APDJuggernaut::Pattern1FireTick, FMath::Max(Pattern1FireInterval, 0.02f), /*bLoop=*/true);
}

void APDJuggernaut::Pattern1FireTick()
{
	Pattern1FireElapsed += FMath::Max(Pattern1FireInterval, 0.02f);

	// 발사 중 조준 방향(Pattern1AimDir)은 FacePatternTarget 이 느린 속도로 갱신 — 총구 방향 = 그 값.

	// 머즐 소켓 번갈아 사용 — VFX 훅 + 발사체 스폰 위치.
	FName Socket = NAME_None;
	FVector MuzzleLoc = GetActorLocation();
	if (Pattern1MuzzleSockets.Num() > 0)
	{
		Socket = Pattern1MuzzleSockets[Pattern1MuzzleIndex % Pattern1MuzzleSockets.Num()];
		if (MachineGunMesh && Socket != NAME_None && MachineGunMesh->DoesSocketExist(Socket))
		{
			MuzzleLoc = MachineGunMesh->GetSocketLocation(Socket);
		}
	}
	++Pattern1MuzzleIndex;

	Multicast_OnPattern1FireShot(Socket, MuzzleLoc);

	SpawnMachineGunProjectile(MuzzleLoc);

	if (Pattern1FireElapsed >= Pattern1FireDuration)
	{
		EndPattern();
	}
}

void APDJuggernaut::SpawnMachineGunProjectile(const FVector& MuzzleLoc)
{
	if (!HasAuthority() || !MachineGunProjectileClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 수평 = 느리게 추적되는 총구 방향(Pattern1AimDir). 수직만 타겟 높이에 맞춰 명중 보장(타겟 없으면 수평 정면).
	FVector Dir = Pattern1AimDir;
	if (const AActor* Target = ActiveTarget.Get())
	{
		const FVector TargetLoc = Target->GetActorLocation();
		const double HorizDist = FVector::Dist2D(MuzzleLoc, TargetLoc);
		if (HorizDist > 1.0)
		{
			const double Slope = (TargetLoc.Z - MuzzleLoc.Z) / HorizDist;
			Dir = FVector(Pattern1AimDir.X, Pattern1AimDir.Y, Slope);
		}
	}
	Dir = Dir.GetSafeNormal();
	if (Dir.IsNearlyZero()) Dir = GetActorForwardVector();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APDProjectile* Projectile = World->SpawnActor<APDProjectile>(
		MachineGunProjectileClass, MuzzleLoc, Dir.Rotation(), SpawnParams);
	if (Projectile)
	{
		Projectile->InitProjectile(Pattern1DamagePerTick, this, /*bPenetrate=*/false, Dir);
	}
}

void APDJuggernaut::StopPattern1Audio()
{
	if (Pattern1ChargeAudio) { Pattern1ChargeAudio->Stop(); Pattern1ChargeAudio = nullptr; }
	if (Pattern1FireAudio)   { Pattern1FireAudio->Stop();   Pattern1FireAudio = nullptr; }
}

void APDJuggernaut::EndPattern()
{
	const EPDJuggernautPattern Finished = CurrentPattern;

	FTimerManager& TM = GetWorldTimerManager();
	TM.ClearTimer(PatternChargeHandle);
	TM.ClearTimer(Pattern1FireHandle);
	TM.ClearTimer(Pattern2LaunchHandle);
	TM.ClearTimer(Pattern2LaunchFallbackHandle);
	Pattern2PendingImpacts.Reset();

	bIsExecutingPattern = false;
	CurrentPattern = EPDJuggernautPattern::None;
	bPattern1Firing = false;
	SyncStateToBlackboard();

	// 종료된 패턴에 맞는 BP 정리 훅 호출.
	switch (Finished)
	{
	case EPDJuggernautPattern::Pulverize:  Multicast_OnPattern1End(); break;
	case EPDJuggernautPattern::Annihilate: Multicast_OnPattern2End(); break;
	default: break;
	}

	// 활성 상태면 다음 패턴 쿨다운 재예약(일반공격은 BT 가 자동 재개).
	if (Phase == EPDJuggernautPhase::Active)
	{
		ScheduleNextPattern();
	}
}

// ───────────────────────── 패턴2: 섬멸 (미사일 스웜) ─────────────────────────

void APDJuggernaut::StartPattern2(AActor* Target)
{
	bIsExecutingPattern = true;
	CurrentPattern = EPDJuggernautPattern::Annihilate;
	SyncStateToBlackboard();

	// 패턴 중 일반공격 중단. 회전(FacePatternTarget)은 Tick 이 담당.
	StopContinuousFire();

	Pattern2LaunchedCount = 0;
	Pattern2VolleyIndex = 0;
	Pattern2PendingImpacts.Reset();

	// 런처 애님 재생(모든 머신). 발사각 도달 프레임의 PDJuggernautLaunchNotify 가 OnPattern2LaunchNotify 호출 → 발사 시작.
	Multicast_OnPattern2Begin(Target);

	// 안전망: 애님/노티파이 미설정(또는 데디서버 노티파이 미발화) 시 발사가 안 시작돼 패턴이 안 끝나는 소프트락 방지.
	GetWorldTimerManager().SetTimer(Pattern2LaunchFallbackHandle, this,
		&APDJuggernaut::OnPattern2LaunchNotify, FMath::Max(Pattern2LaunchFallbackDelay, 0.1f), /*bLoop=*/false);
}

void APDJuggernaut::OnPattern2LaunchNotify()
{
	// 발사 게임플레이는 서버 전용(노티파이는 모든 머신에서 발화하므로 가드 필수).
	if (!HasAuthority()) return;
	if (CurrentPattern != EPDJuggernautPattern::Annihilate || !bIsExecutingPattern) return;
	if (Pattern2LaunchedCount > 0) return;   // 이미 시작됨 — 노티파이/폴백 중복 무시.

	GetWorldTimerManager().ClearTimer(Pattern2LaunchFallbackHandle);

	// 첫 볼리 즉시 + 이후 볼리 stagger 발사(볼리 = 짝지은 해치 동시 발사).
	Pattern2LaunchVolley();
	if (Pattern2LaunchedCount < Pattern2MissileCount)
	{
		GetWorldTimerManager().SetTimer(Pattern2LaunchHandle, this,
			&APDJuggernaut::Pattern2LaunchVolley, FMath::Max(Pattern2LaunchInterval, 0.02f), /*bLoop=*/true);
	}
}

void APDJuggernaut::Pattern2LaunchVolley()
{
	// 한 볼리 = Pattern2MissilesPerVolley 발 동시 발사. 해치 짝 = v, v+NumVolleys, ...
	// (예: 해치 8·볼리당 2발 → 0&4, 1&5, 2&6, 3&7 순으로 stagger.)
	const int32 PerVolley  = FMath::Max(Pattern2MissilesPerVolley, 1);
	const int32 NumVolleys = FMath::Max(FMath::DivideAndRoundUp(Pattern2MissileCount, PerVolley), 1);

	for (int32 m = 0; m < PerVolley && Pattern2LaunchedCount < Pattern2MissileCount; ++m)
	{
		Pattern2LaunchMissile(Pattern2VolleyIndex + m * NumVolleys);
	}

	++Pattern2VolleyIndex;
	if (Pattern2LaunchedCount >= Pattern2MissileCount || Pattern2VolleyIndex >= NumVolleys)
	{
		GetWorldTimerManager().ClearTimer(Pattern2LaunchHandle);
	}
}

void APDJuggernaut::Pattern2LaunchMissile(int32 HatchIndex)
{
	// 착탄 중심 = 발사 시점의 플레이어 위치(스웜이 이동을 어느 정도 따라감). 타겟 무효 시 전방 폴백.
	const AActor* Target = ActiveTarget.Get();
	const FVector Center = Target
		? Target->GetActorLocation()
		: GetActorLocation() + GetActorForwardVector() * (Pattern1MaxRange + Pattern2ScatterRadius);

	// 이미 예약된 착탄과 떨어지도록 분산 선정 — 한곳에 뭉쳐 AoE 가 겹치는 문제 완화.
	// 플레이어 위치는 캡슐 중심(바닥 위)이라 그대로 쓰면 공중에서 터짐 → 바닥으로 Z 스냅.
	const FVector Impact = SnapImpactToGround(PickScatterImpactLocation(Center));

	// 지정 해치 소켓에서 발사.
	FName Hatch = NAME_None;
	FVector LaunchLoc = GetActorLocation();
	if (Pattern2HatchSockets.Num() > 0)
	{
		Hatch = Pattern2HatchSockets[HatchIndex % Pattern2HatchSockets.Num()];
		if (MissileLauncherMesh && Hatch != NAME_None && MissileLauncherMesh->DoesSocketExist(Hatch))
		{
			LaunchLoc = MissileLauncherMesh->GetSocketLocation(Hatch);
		}
	}

	Multicast_OnPattern2Launch(Hatch, LaunchLoc, Impact, Pattern2TravelTime);

	// 착탄 예약 — Tick 이 world-time 으로 처리(개별 타이머 수명 이슈 회피).
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Pattern2PendingImpacts.Add({ Impact, Now + Pattern2TravelTime });

	++Pattern2LaunchedCount;
}

FVector APDJuggernaut::PickScatterImpactLocation(const FVector& Center) const
{
	// 디스크 내 균등 랜덤 후보 생성.
	auto RandomInDisk = [&]() -> FVector
	{
		const float Angle  = FMath::FRandRange(0.f, 2.f * PI);
		const float Radius = Pattern2ScatterRadius * FMath::Sqrt(FMath::FRand());
		return Center + FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.f);
	};

	// 첫 발은 비교 대상이 없으니 즉시 반환.
	if (Pattern2PendingImpacts.Num() == 0)
	{
		return RandomInDisk();
	}

	// best-candidate: 후보 여러 개 중 기존 착탄과 가장 멀리 떨어진 지점 선택.
	// MinSeparation 충족 시 즉시 채택, 아니면 가장 떨어진 후보로 폴백(공간이 좁아도 무한루프 없음).
	const double MinSepSq = static_cast<double>(Pattern2MinSeparation) * Pattern2MinSeparation;
	constexpr int32 NumCandidates = 16;

	FVector BestPoint = Center;
	double   BestNearestSq = -1.0;

	for (int32 c = 0; c < NumCandidates; ++c)
	{
		const FVector Cand = RandomInDisk();

		double NearestSq = TNumericLimits<double>::Max();
		for (const FPDPendingImpact& P : Pattern2PendingImpacts)
		{
			NearestSq = FMath::Min(NearestSq, FVector::DistSquared2D(Cand, P.Location));
		}

		if (NearestSq >= MinSepSq)
		{
			return Cand;
		}
		if (NearestSq > BestNearestSq)
		{
			BestNearestSq = NearestSq;
			BestPoint = Cand;
		}
	}
	return BestPoint;
}

FVector APDJuggernaut::SnapImpactToGround(const FVector& In) const
{
	const UWorld* World = GetWorld();
	if (!World) return In;

	// 후보 XY 에서 위→아래 라인트레이스로 바닥 Z 확정. 미검출 시 원본 유지.
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(JuggernautImpactGround), /*bTraceComplex=*/false, this);
	if (World->LineTraceSingleByChannel(Hit, In + FVector(0.f, 0.f, 1000.f), In - FVector(0.f, 0.f, 5000.f), ECC_WorldStatic, Params))
	{
		return Hit.ImpactPoint;
	}
	return In;
}

void APDJuggernaut::TickPattern2Impacts()
{
	const UWorld* World = GetWorld();
	if (!World) return;

	const float Now = World->GetTimeSeconds();

	// 시간 도달분 폭발 처리(역순 제거 안전).
	for (int32 i = Pattern2PendingImpacts.Num() - 1; i >= 0; --i)
	{
		if (Now >= Pattern2PendingImpacts[i].ImpactTime)
		{
			const FVector Loc = Pattern2PendingImpacts[i].Location;
			Pattern2PendingImpacts.RemoveAt(i);
			Multicast_OnPattern2Impact(Loc);
			ApplyPattern2Damage(Loc);
		}
	}

	// 모두 발사 + 모두 착탄 → 패턴 종료.
	if (Pattern2LaunchedCount >= Pattern2MissileCount && Pattern2PendingImpacts.Num() == 0)
	{
		EndPattern();
	}
}

void APDJuggernaut::ApplyPattern2Damage(const FVector& Center)
{
	if (!HasAuthority()) return;

	const UWorld* World = GetWorld();
	if (!World) return;

	const float RadiusSq = Pattern2ImpactRadius * Pattern2ImpactRadius;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
		if (!PlayerPawn || PlayerPawn == this) continue;
		if (!PlayerPawn->Implements<UPDDamageable>()) continue;
		if (!IPDDamageable::Execute_IsAlive(PlayerPawn)) continue;

		// 착탄 원(2D) 안의 플레이어만 피해 — 탑뷰 기준.
		if ((PlayerPawn->GetActorLocation() - Center).SizeSquared2D() > RadiusSq) continue;

		// 부위별 시스템 대응: 부위마다 매핑되는 본을 지정해 각 부위에 개별 ApplyDamage.
		// (HitResult.BoneName → 대상 BodyPartConfig 가 부위로 라우팅. 코어 데미지 경로는 변경 없음.)
		if (Pattern2ImpactBones.Num() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PD Missile] ApplyPattern2Damage -> %s | bones=%d"),
				*GetNameSafe(PlayerPawn), Pattern2ImpactBones.Num());
			for (const FName& Bone : Pattern2ImpactBones)
			{
				// 본별 오버라이드 우선, 없으면 기본 데미지.
				const float* Override = Pattern2BoneDamageOverrides.Find(Bone);
				const float BoneDamage = Override ? *Override : Pattern2Damage;

				FPDDamageInfo DamageInfo;
				DamageInfo.BaseDamage = BoneDamage;
				DamageInfo.Instigator = this;
				DamageInfo.HitResult.BoneName = Bone;
				UE_LOG(LogTemp, Warning, TEXT("[PD Missile]   bone=%s dmg=%.0f"), *Bone.ToString(), BoneDamage);
				IPDDamageable::Execute_ApplyDamage(PlayerPawn, DamageInfo);
			}
		}
		else
		{
			// 본 목록 미설정 시 단일 부위(Torso) 기본 동작.
			FPDDamageInfo DamageInfo;
			DamageInfo.BaseDamage = Pattern2Damage;
			DamageInfo.Instigator = this;
			IPDDamageable::Execute_ApplyDamage(PlayerPawn, DamageInfo);
		}
	}
}

// ─────────────────────────── 연출 멀티캐스트 ───────────────────────────
// 서버가 호출 → 리슨 호스트 포함 전 머신에서 디자이너 BP 훅 발화. 데미지·타이밍은 서버가 이미 처리.

void APDJuggernaut::Multicast_OnActivated_Implementation(AActor* Target)                { BP_OnActivated(Target); }
void APDJuggernaut::Multicast_OnBeginDeactivation_Implementation()                      { BP_OnBeginDeactivation(); }
void APDJuggernaut::Multicast_OnDeactivated_Implementation()                            { BP_OnDeactivated(); }
void APDJuggernaut::Multicast_OnReturnHomeComplete_Implementation()                     { BP_OnReturnHomeComplete(); }
void APDJuggernaut::Multicast_OnPattern1BeginAim_Implementation(AActor* Target)
{
	// 차징 시작부터 머신건 회전(스핀업) 애니메이션 루프 — 발사까지 끊김 없이 이어짐. 모든 머신에서 재생.
	if (MachineGunMesh && MachineGunFireAnim)
	{
		MachineGunMesh->PlayAnimation(MachineGunFireAnim, /*bLooping=*/true);
	}
	// 차징 사운드(스핀업) — 머신건에 부착 재생.
	if (Pattern1ChargeSound)
	{
		USceneComponent* AttachComp = MachineGunMesh ? MachineGunMesh.Get() : GetRootComponent();
		Pattern1ChargeAudio = UGameplayStatics::SpawnSoundAttached(Pattern1ChargeSound, AttachComp);
	}
	BP_OnPattern1BeginAim(Target);
}
void APDJuggernaut::Multicast_OnPattern1BeginFire_Implementation(FVector AimDirection)
{
	// 차징 사운드 정지 → 발사 루프 사운드 시작.
	if (Pattern1ChargeAudio) { Pattern1ChargeAudio->Stop(); Pattern1ChargeAudio = nullptr; }
	if (Pattern1FireLoopSound)
	{
		USceneComponent* AttachComp = MachineGunMesh ? MachineGunMesh.Get() : GetRootComponent();
		Pattern1FireAudio = UGameplayStatics::SpawnSoundAttached(Pattern1FireLoopSound, AttachComp);
	}
	BP_OnPattern1BeginFire(AimDirection);
}
void APDJuggernaut::Multicast_OnPattern1FireShot_Implementation(FName MuzzleSocket, FVector MuzzleLocation)
{
	// 머즐 플래시 파티클을 머신건 머즐 소켓에 부착 스폰(라이플 GCN_Weapon_Fire 의 SpawnEmitterAttached 와 동일). 모든 머신.
	if (MuzzleFlashFX && MachineGunMesh)
	{
		UGameplayStatics::SpawnEmitterAttached(
			MuzzleFlashFX, MachineGunMesh, MuzzleSocket,
			FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget, /*bAutoDestroy=*/true);
	}
	BP_OnPattern1FireShot(MuzzleSocket, MuzzleLocation);
}
void APDJuggernaut::Multicast_OnPattern1End_Implementation()
{
	if (MachineGunMesh && MachineGunFireAnim)
	{
		MachineGunMesh->Stop();
	}
	StopPattern1Audio();
	BP_OnPattern1End();
}
void APDJuggernaut::Multicast_OnPattern2Begin_Implementation(AActor* Target)
{
	// 런처 애님 원샷 재생(발사각으로 raise). 14프레임 등의 발사 노티파이가 발사를 트리거. 모든 머신.
	if (MissileLauncherMesh && Pattern2LauncherAnim)
	{
		MissileLauncherMesh->PlayAnimation(Pattern2LauncherAnim, /*bLooping=*/false);
	}
	BP_OnPattern2Begin(Target);
}
void APDJuggernaut::Multicast_OnPattern2Launch_Implementation(FName HatchSocket, FVector LaunchLocation, FVector ImpactLocation, float TravelTime)
{
	// 코스메틱 미사일 스폰(렌더되는 머신만 — 데디서버 제외). 데미지는 서버가 착탄 시 AoE 로 처리.
	if (Pattern2MissileClass && GetNetMode() != NM_DedicatedServer)
	{
		if (UWorld* World = GetWorld())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (APDJuggernautMissile* Missile = World->SpawnActor<APDJuggernautMissile>(
					Pattern2MissileClass, LaunchLocation, FRotator::ZeroRotator, SpawnParams))
			{
				Missile->InitMissile(LaunchLocation, ImpactLocation, TravelTime, Pattern2ImpactRadius);
			}
		}
	}
	// 발사 사운드 — 해치 위치에서 1회 재생(데디서버는 PlaySoundAtLocation 이 무음 처리).
	if (Pattern2LaunchSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Pattern2LaunchSound, LaunchLocation);
	}

	BP_OnPattern2Launch(HatchSocket, LaunchLocation, ImpactLocation, TravelTime);
}
void APDJuggernaut::Multicast_OnPattern2Impact_Implementation(FVector ImpactLocation)   { BP_OnPattern2Impact(ImpactLocation); }
void APDJuggernaut::Multicast_OnPattern2End_Implementation()                            { BP_OnPattern2End(); }

// ─────────────────────────── 타겟 / BB 동기화 ───────────────────────────

APDEnemyAIControllerBase* APDJuggernaut::GetEnemyController() const
{
	return Cast<APDEnemyAIControllerBase>(GetController());
}

void APDJuggernaut::SetCombatTarget(AActor* Target)
{
	// CombatComponent 경유 — RequestAttack 의 타겟/사거리 검증 + AIController 가 BB.TargetActor 로 동기화.
	if (UPDCombatComponent* Combat = GetCombatComponent())
	{
		if (Target) Combat->SetCurrentTarget(Target);
		else        Combat->ClearCurrentTarget();
	}
}

void APDJuggernaut::SyncStateToBlackboard()
{
	APDEnemyAIControllerBase* AICon = GetEnemyController();
	UBlackboardComponent* BB = AICon ? AICon->GetBlackboard() : nullptr;
	if (!BB) return;

	BB->SetValueAsEnum(PDBTKeys::BossPhase, static_cast<uint8>(Phase));
	BB->SetValueAsBool(PDBTKeys::bIsExecutingPattern, bIsExecutingPattern);

	// EnemyState 도 동기화 — perception 의 HandleTargetLost 가 BB.EnemyState 로 교전 여부를 판단해
	// 비-교전 상태면 TargetActor 를 클리어하므로, 보스의 CombatComponent 타겟과 충돌하지 않도록 맞춰둠.
	BB->SetValueAsEnum(PDBTKeys::EnemyState, static_cast<uint8>(GetEnemyState()));
}

// ───────────────────────────────── 디버그 ─────────────────────────────────

void APDJuggernaut::DrawBossDebug() const
{
#if ENABLE_DRAW_DEBUG
	UWorld* World = GetWorld();
	if (!World) return;

	const FVector Origin = GetActorLocation();
	const FVector Forward = GetActorForwardVector();

	// 탑뷰 수평 원 — 컨트롤러 디버그와 동일한 XY 평면 정렬.
	const FVector XAxis(1.f, 0.f, 0.f);
	const FVector YAxis(0.f, 1.f, 0.f);

	// 단계별 색상.
	FColor PhaseColor = FColor::White;
	const TCHAR* PhaseName = TEXT("?");
	switch (Phase)
	{
	case EPDJuggernautPhase::Dormant:      PhaseColor = FColor(120, 120, 120); PhaseName = TEXT("Dormant");      break;
	case EPDJuggernautPhase::Active:       PhaseColor = FColor::Green;          PhaseName = TEXT("Active");       break;
	case EPDJuggernautPhase::Deactivating: PhaseColor = FColor(255, 140, 0);    PhaseName = TEXT("Deactivating"); break;
	case EPDJuggernautPhase::Returning:    PhaseColor = FColor::Cyan;           PhaseName = TEXT("Returning");    break;
	}

	// 활성/비활성 경계(3000) — 단계 색으로. 패턴 사거리(Pattern1Range)가 다르면 별도 표시.
	DrawDebugCircle(World, Origin, ActivationRange, 64, PhaseColor, false, -1.f, SDPG_World, 3.f, XAxis, YAxis, false);
	DrawDebugCircle(World, Origin, Pattern1MaxRange, 48, FColor::Yellow, false, -1.f, SDPG_World, 2.f, XAxis, YAxis, false);
	if (!FMath::IsNearlyEqual(Pattern1Range, ActivationRange))
	{
		DrawDebugCircle(World, Origin, Pattern1Range, 48, FColor(200, 0, 0), false, -1.f, SDPG_World, 1.f, XAxis, YAxis, false);
	}

	// 본체 위치/정면.
	DrawDebugSphere(World, Origin + FVector(0, 0, 50.f), 50.f, 12, PhaseColor, false, -1.f, SDPG_World, 2.f);
	DrawDebugDirectionalArrow(World, Origin + FVector(0, 0, 50.f), Origin + Forward * 300.f + FVector(0, 0, 50.f),
		120.f, PhaseColor, false, -1.f, SDPG_World, 3.f);

	// 원위치(HomeLocation) — Returning 목적지. BB 에서 읽음.
	if (const APDEnemyAIControllerBase* AICon = GetEnemyController())
	{
		if (const UBlackboardComponent* BB = AICon->GetBlackboardComponent())
		{
			const FVector Home = BB->GetValueAsVector(PDBTKeys::HomeLocation);
			if (!Home.ContainsNaN() && FMath::IsFinite(Home.X))
			{
				DrawDebugSphere(World, Home, HomeArrivalTolerance, 16, FColor::Blue, false, -1.f, SDPG_World, 1.5f);
				DrawDebugString(World, Home + FVector(0, 0, 60.f), TEXT("HOME"), nullptr, FColor::Blue, 0.f, false, 1.2f);
				DrawDebugLine(World, Origin, Home, FColor::Blue, false, -1.f, SDPG_World, 1.f);
			}
		}
	}

	// 현재 타겟까지 선 + 거리.
	const AActor* Target = ActiveTarget.Get();
	if (Target)
	{
		const FVector TargetLoc = Target->GetActorLocation();
		const float Dist = FVector::Dist(Origin, TargetLoc);
		DrawDebugLine(World, Origin, TargetLoc, FColor::Red, false, -1.f, SDPG_World, 2.f);
		DrawDebugString(World, (Origin + TargetLoc) * 0.5f + FVector(0, 0, 80.f),
			FString::Printf(TEXT("%.0f"), Dist), nullptr, FColor::Red, 0.f, false, 1.2f);
	}

	// 패턴별 시각화.
	if (bIsExecutingPattern && CurrentPattern == EPDJuggernautPattern::Pulverize)
	{
		// 발사 중이면 잠긴 조준 방향(추적), 차징 중이면 타겟 방향.
		const bool bFiring = World->GetTimerManager().IsTimerActive(Pattern1FireHandle);
		FVector ConeDir = Pattern1AimDir;
		if (!bFiring && Target)
		{
			const FVector ToTarget = (Target->GetActorLocation() - Origin).GetSafeNormal2D();
			if (!ToTarget.IsNearlyZero()) ConeDir = ToTarget;
		}

		const FVector Apex = Origin + FVector(0, 0, 60.f);
		const float HalfAngleRad = FMath::DegreesToRadians(Pattern1ConeHalfAngleDeg);
		const FColor ConeColor = bFiring ? FColor::Red : FColor(255, 120, 0); // 발사=빨강, 조준=주황
		DrawDebugCone(World, Apex, ConeDir, Pattern1Range, HalfAngleRad, HalfAngleRad, 24, ConeColor, false, -1.f, SDPG_World, 2.f);

		// 머신건 머즐 소켓 위치.
		if (MachineGunMesh)
		{
			for (const FName& Socket : Pattern1MuzzleSockets)
			{
				if (Socket != NAME_None && MachineGunMesh->DoesSocketExist(Socket))
				{
					DrawDebugSphere(World, MachineGunMesh->GetSocketLocation(Socket), 12.f, 8, FColor::Magenta, false, -1.f, SDPG_World, 1.5f);
				}
			}
		}
	}
	else if (bIsExecutingPattern && CurrentPattern == EPDJuggernautPattern::Annihilate)
	{
		const float Now = World->GetTimeSeconds();
		for (const FPDPendingImpact& Impact : Pattern2PendingImpacts)
		{
			// 착탄까지 차오름 비율(인디케이터). 0=막 발사, 1=착탄 직전.
			const float Remaining = Impact.ImpactTime - Now;
			const float Fill = FMath::Clamp(1.f - Remaining / FMath::Max(Pattern2TravelTime, KINDA_SMALL_NUMBER), 0.f, 1.f);

			DrawDebugCircle(World, Impact.Location, Pattern2ImpactRadius, 32, FColor::Red, false, -1.f, SDPG_World, 2.f, XAxis, YAxis, false);
			DrawDebugCircle(World, Impact.Location, Pattern2ImpactRadius * Fill, 24, FColor(255, 80, 0), false, -1.f, SDPG_World, 2.f, XAxis, YAxis, false);
		}

		// 발사 해치 소켓 위치.
		if (MissileLauncherMesh)
		{
			for (const FName& Hatch : Pattern2HatchSockets)
			{
				if (Hatch != NAME_None && MissileLauncherMesh->DoesSocketExist(Hatch))
				{
					DrawDebugSphere(World, MissileLauncherMesh->GetSocketLocation(Hatch), 10.f, 8, FColor::Magenta, false, -1.f, SDPG_World, 1.f);
				}
			}
		}
	}

	// ── 화면 텍스트(GEngine) — 보스마다 고유 키로 갱신(중복 보스/타 시스템과 충돌 방지). ──
	if (!GEngine) return;

	const uint64 KeyBase = static_cast<uint64>(GetUniqueID()) * 100;
	FTimerManager& TM = World->GetTimerManager();

	auto Msg = [&](int32 Line, const FColor& Col, const FString& Text)
	{
		GEngine->AddOnScreenDebugMessage(KeyBase + Line, 1.1f, Col, Text);
	};

	Msg(0, FColor::White, TEXT("===== JUGGERNAUT ====="));
	Msg(1, PhaseColor, FString::Printf(TEXT("Phase: %s"), PhaseName));

	const TCHAR* PatternName = TEXT("None");
	switch (CurrentPattern)
	{
	case EPDJuggernautPattern::Pulverize:  PatternName = TEXT("Pulverize(MG)");  break;
	case EPDJuggernautPattern::Annihilate: PatternName = TEXT("Annihilate(Swarm)"); break;
	default: break;
	}
	Msg(2, bIsExecutingPattern ? FColor::Red : FColor(150, 150, 150),
		FString::Printf(TEXT("Pattern: %s  Executing=%s"), PatternName, bIsExecutingPattern ? TEXT("Y") : TEXT("N")));

	if (Target)
	{
		Msg(3, FColor::Orange, FString::Printf(TEXT("Target: %s  Dist=%.0f"), *Target->GetName(), FVector::Dist(Origin, Target->GetActorLocation())));
	}
	else
	{
		Msg(3, FColor(150, 150, 150), TEXT("Target: none"));
	}

	// 타이밍 — 패턴 쿨다운 / 차징·발사·착탄 / 비활성 카운트다운.
	if (Phase == EPDJuggernautPhase::Deactivating)
	{
		Msg(4, FColor(255, 140, 0), FString::Printf(TEXT("Deactivate in: %.1fs"), FMath::Max(TM.GetTimerRemaining(DeactivationTimerHandle), 0.f)));
	}
	else if (bIsExecutingPattern && CurrentPattern == EPDJuggernautPattern::Pulverize)
	{
		const float ChargeRemain = TM.GetTimerRemaining(PatternChargeHandle);
		if (ChargeRemain > 0.f) Msg(4, FColor::Yellow, FString::Printf(TEXT("Charging: %.1fs"), ChargeRemain));
		else                    Msg(4, FColor::Red, FString::Printf(TEXT("Firing: %.1f/%.1fs"), Pattern1FireElapsed, Pattern1FireDuration));
	}
	else if (bIsExecutingPattern && CurrentPattern == EPDJuggernautPattern::Annihilate)
	{
		Msg(4, FColor::Red, FString::Printf(TEXT("Swarm: launched=%d/%d  pending=%d"), Pattern2LaunchedCount, Pattern2MissileCount, Pattern2PendingImpacts.Num()));
	}
	else
	{
		const float Cd = TM.GetTimerRemaining(PatternCooldownHandle);
		Msg(4, FColor::Cyan, Cd > 0.f ? FString::Printf(TEXT("Next pattern in: %.1fs"), Cd) : TEXT("Pattern: idle"));
	}

	// 부위별 HP — 풀회복/피격 검증용.
	if (AttributeSet)
	{
		Msg(5, FColor::Green, FString::Printf(TEXT("HP H:%.0f T:%.0f AL:%.0f AR:%.0f LL:%.0f LR:%.0f"),
			AttributeSet->GetHeadHP(), AttributeSet->GetTorsoHP(),
			AttributeSet->GetArmLHP(), AttributeSet->GetArmRHP(),
			AttributeSet->GetLegLHP(), AttributeSet->GetLegRHP()));
	}
#endif
}
