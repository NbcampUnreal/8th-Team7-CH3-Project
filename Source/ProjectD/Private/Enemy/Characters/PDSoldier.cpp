#include "Enemy/Characters/PDSoldier.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/PDAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Enemy/AI/Controllers/PDEnemyAIControllerBase.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "GameplayTag/PDGameplayTags.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "Weapons/Base/PDRangedWeaponBase.h"

#if ENABLE_DRAW_DEBUG
static IConsoleVariable* GetPDAIDebugCVar_Soldier()
{
	static IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("pd.ai.debugdraw"));
	return CVar;
}
#define PD_SOLDIER_FIRE_LOG(Reason, ...) \
	do { const IConsoleVariable* CVar = GetPDAIDebugCVar_Soldier(); \
	     if (CVar && CVar->GetInt() != 0) { UE_LOG(LogPDAI, Log, TEXT("[%s] OnFireTick: " Reason), *GetName(), ##__VA_ARGS__); } } while(0)
#else
#define PD_SOLDIER_FIRE_LOG(Reason, ...)
#endif

APDSoldier::APDSoldier()
{
	TeamID = 2; // Hostile.
}

void APDSoldier::BeginPlay()
{
	Super::BeginPlay();

	// 무기 타입 태그 변경 시 AnimLayer 갈아끼우기 — Player 와 동일 패턴.
	if (ASC)
	{
		ASC->RegisterGameplayTagEvent(PDGameplayTags::Weapon_Type_Rifle,   EGameplayTagEventType::NewOrRemoved).AddUObject(this, &APDSoldier::OnWeaponTypeTagChanged);
		ASC->RegisterGameplayTagEvent(PDGameplayTags::Weapon_Type_Shotgun, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &APDSoldier::OnWeaponTypeTagChanged);
		ASC->RegisterGameplayTagEvent(PDGameplayTags::Weapon_Type_Sniper,  EGameplayTagEventType::NewOrRemoved).AddUObject(this, &APDSoldier::OnWeaponTypeTagChanged);
	}
	LinkDefaultAnimLayer();

	SpawnAndEquipDefaultWeapon();

	// 타겟 획득/상실에 맞춰 풀오토 루프 on/off.
	// OnAttackRequested 는 단발/비-Rifle 경로 — BT FireAtTarget 매 cycle 1회 발사.
	if (UPDCombatComponent* Combat = GetCombatComponent())
	{
		Combat->OnTargetChanged  .AddDynamic(this, &APDSoldier::HandleTargetChanged);
		Combat->OnAttackRequested.AddDynamic(this, &APDSoldier::HandleAttackRequested);
	}
}

void APDSoldier::OnWeaponTypeTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount == 0) return;

	APDWeaponBase* CurWeapon = GetCurrentWeapon();
	if (!IsValid(CurWeapon)) return;

	TSubclassOf<UAnimInstance> LayerClass = CurWeapon->GetWeaponAnimLayerClass();
	if (!LayerClass) { LinkDefaultAnimLayer(); return; }

	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->LinkAnimClassLayers(LayerClass);
	}
}

void APDSoldier::LinkDefaultAnimLayer()
{
	if (!DefaultAnimLayerClass) return;
	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->LinkAnimClassLayers(DefaultAnimLayerClass);
	}
}

void APDSoldier::OnEnterState_Dead()
{
	StopContinuousFire();

	// 무기 → 시체 Stash 이전은 EnemyBase::OnEnterState_Dead 안의 TryDropEquippedWeaponToCorpse 가
	// WeaponDropChance 확률로 처리. 본 클래스는 발사 timer 정리 + 무기 액터 정리만 담당.
	Super::OnEnterState_Dead();

	if (!EquippedWeapon) return;

	EquippedWeapon->OnUnequip();
	EquippedWeapon->Destroy();
	EquippedWeapon = nullptr;
}

FName APDSoldier::GetEquippedWeaponItemID_Implementation() const
{
	return EquippedWeapon ? EquippedWeapon->GetItemID() : NAME_None;
}

void APDSoldier::SpawnAndEquipDefaultWeapon()
{
	if (!DefaultWeaponClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APDWeaponBase* NewWeapon = World->SpawnActor<APDWeaponBase>(
		DefaultWeaponClass,
		GetActorLocation(),
		GetActorRotation(),
		SpawnParams);

	if (!NewWeapon) return;

	SetEquippedWeapon(NewWeapon, /*bDestroyPrevious=*/true);
}

void APDSoldier::SetEquippedWeapon(APDWeaponBase* NewWeapon, bool bDestroyPrevious)
{
	if (EquippedWeapon == NewWeapon) return;

	UPDAnimInstance* AnimInst = GetMesh() ? Cast<UPDAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr;

	if (EquippedWeapon)
	{
		// 무기 타입 태그 제거 — AnimInstance.WeaponType 캐시가 None 으로 복귀하도록.
		if (ASC)
		{
			ASC->RemoveLooseGameplayTag(EquippedWeapon->GetWeaponTypeTag());
		}
		if (AnimInst)
		{
			AnimInst->OnWeaponUnequipped(Cast<APDRangedWeaponBase>(EquippedWeapon));
		}

		EquippedWeapon->OnUnequip();
		if (bDestroyPrevious)
		{
			EquippedWeapon->Destroy();
		}
	}

	EquippedWeapon = NewWeapon;

	if (EquippedWeapon)
	{
		AttachActorToWeaponSocket(EquippedWeapon);
		EquippedWeapon->OnEquip(this);

		// 무한탄약 옵션 전파: Soldier가 든 사격무기는 인벤토리 없이도 장전 시 풀충.
		if (APDRangedWeaponBase* Ranged = Cast<APDRangedWeaponBase>(EquippedWeapon))
		{
			Ranged->SetInfiniteAmmo(bForceInfiniteAmmo);
		}

		// AnimInstance 가 ASC 태그를 보고 WeaponType/bIsMelee 를 결정 — 태그를 먼저 박는다.
		if (ASC)
		{
			ASC->AddLooseGameplayTag(EquippedWeapon->GetWeaponTypeTag());
		}

		// Fire/Reload/Equip 몽타주 바인딩 + Equip 몽타주 즉시 재생.
		if (AnimInst)
		{
			AnimInst->OnWeaponEquipped(Cast<APDRangedWeaponBase>(EquippedWeapon));
		}
	}

	// 무기 변경 후 연사 timer 재평가 — Rifle Auto↔비-Rifle/단발 전환 즉시 반영.
	// OnTargetChanged 는 무기 변경 시 호출되지 않으므로 여기서 강제 재평가.
	if (UPDCombatComponent* Combat = GetCombatComponent())
	{
		if (Combat->HasValidTarget() && EquippedWeapon && IsCurrentWeaponFullAutoMode())
		{
			StartContinuousFire();
		}
		else
		{
			StopContinuousFire();
		}
	}
}

void APDSoldier::HandleTargetChanged(AActor* NewTarget)
{
	PD_SOLDIER_FIRE_LOG("HandleTargetChanged NewTarget=%s, bAutoFire=%s, HasWeapon=%s, FullAuto=%s",
		*GetNameSafe(NewTarget),
		bAutoFireOnAttackRequested ? TEXT("Y") : TEXT("N"),
		EquippedWeapon ? TEXT("Y") : TEXT("N"),
		IsCurrentWeaponFullAutoMode() ? TEXT("Y") : TEXT("N"));

	if (!bAutoFireOnAttackRequested) return;

	// 연사 timer 는 Rifle Auto 에서만 사용. 단발/비-Rifle 은 BT 의 RequestAttack 단발 경로로.
	if (NewTarget && EquippedWeapon && IsCurrentWeaponFullAutoMode())
	{
		StartContinuousFire();
	}
	else
	{
		StopContinuousFire();
	}
}

void APDSoldier::HandleAttackRequested(AActor* /*Target*/)
{
	// 단발/비-Rifle 무기 경로. Rifle Auto 는 timer 가 처리하므로 본 경로는 no-op.
	if (!bAutoFireOnAttackRequested) return;
	if (!EquippedWeapon) return;
	if (GetEnemyState() != EPDEnemyState::Combat) return;

	if (IsCurrentWeaponFullAutoMode())
	{
		// Rifle Auto 는 timer 가 발사 — 중복 방지.
		// 단, BT 진입 직후 timer 가 아직 시작되지 않았을 가능성에 대비해 한 번 보장.
		UWorld* World = GetWorld();
		if (World && !World->GetTimerManager().IsTimerActive(FireTimerHandle))
		{
			StartContinuousFire();
		}
		return;
	}

	TryFireSingleShot();
}

bool APDSoldier::IsCurrentWeaponFullAutoMode() const
{
	const APDRangedWeaponBase* Ranged = Cast<APDRangedWeaponBase>(EquippedWeapon);
	return Ranged && Ranged->IsFullAuto();
}

bool APDSoldier::TryFireSingleShot()
{
	// OnFireTick 의 발사 직전 검증과 동일 정책 (장전/우군 사선) — 단발 경로에도 적용.
	if (APDRangedWeaponBase* Ranged = Cast<APDRangedWeaponBase>(EquippedWeapon))
	{
		if (Ranged->IsReloading())
		{
			PD_SOLDIER_FIRE_LOG("SINGLE SKIP (reloading)");
			return false;
		}
		if (Ranged->GetCurrentAmmo() <= 0)
		{
			PD_SOLDIER_FIRE_LOG("SINGLE RELOAD (ammo=0)");
			Ranged->Reload();
			return false;
		}
	}

	if (UPDCombatComponent* Combat = GetCombatComponent())
	{
		if (Combat->IsFriendlyInLineOfFire())
		{
			PD_SOLDIER_FIRE_LOG("SINGLE SKIP (friendly in LOF)");
			return false;
		}
	}

	PD_SOLDIER_FIRE_LOG("SINGLE FIRE");
	EquippedWeapon->Fire();
	return true;
}

void APDSoldier::StartContinuousFire()
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (World->GetTimerManager().IsTimerActive(FireTimerHandle)) return;

	// 첫 발은 지연 없이 즉시 시도.
	OnFireTick();

	// 플레이어 GA_FireAbility 와 동일하게 무기 stat 의 FireRate 사용.
	// 미설정/0 이면 FireInterval(BP 디폴트) 로 폴백.
	float Interval = FireInterval;
	if (const APDRangedWeaponBase* Ranged = Cast<APDRangedWeaponBase>(EquippedWeapon))
	{
		const float WeaponRate = Ranged->GetCurrentStats().FireRate;
		if (WeaponRate > 0.f) Interval = WeaponRate;
	}
	Interval = FMath::Max(Interval, 0.0167f); // 1 프레임 클램프.

	World->GetTimerManager().SetTimer(FireTimerHandle, this, &APDSoldier::OnFireTick, Interval, /*bLoop=*/true);
}

void APDSoldier::StopContinuousFire()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimerHandle);
	}
}

void APDSoldier::OnFireTick()
{
	if (!EquippedWeapon)
	{
		PD_SOLDIER_FIRE_LOG("STOP (no weapon)");
		StopContinuousFire();
		return;
	}

	// 무기 교체/FireMode 토글로 더 이상 연사 대상이 아니면 timer 정리 — 단발 경로(HandleAttackRequested)로 위임.
	if (!IsCurrentWeaponFullAutoMode())
	{
		PD_SOLDIER_FIRE_LOG("STOP (weapon not full-auto)");
		StopContinuousFire();
		return;
	}

	UPDCombatComponent* Combat = GetCombatComponent();
	if (!Combat || !Combat->HasValidTarget())
	{
		PD_SOLDIER_FIRE_LOG("STOP (no valid target)");
		StopContinuousFire();
		return;
	}

	// 상태 게이트 — BT 가 Combat 분기에서 SetEnemyState(Combat) 호출했을 때만 발사.
	// Idle/Alert/Chase 등 BT 결정 이전 단계에서 OnTargetChanged 만으로 자동 발사되는 것 차단.
	if (GetEnemyState() != EPDEnemyState::Combat)
	{
		PD_SOLDIER_FIRE_LOG("SKIP (state != Combat, state=%d)", static_cast<int32>(GetEnemyState()));
		return;
	}

	if (bRequireInRangeToFire)
	{
		if (const AActor* Target = Combat->GetCurrentTarget())
		{
			const float Range = Combat->GetAttackRange();
			const float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
			if (DistSq > Range * Range)
			{
				PD_SOLDIER_FIRE_LOG("SKIP (out of range Dist=%.0f Range=%.0f)", FMath::Sqrt(DistSq), Range);
				return;
			}
		}
	}

	// 탄 소진 시 자동 장전: 모션 재생 후 FinishReload에서 풀충 → 사실상 무한 사격 사이클.
	if (APDRangedWeaponBase* Ranged = Cast<APDRangedWeaponBase>(EquippedWeapon))
	{
		if (Ranged->IsReloading())
		{
			PD_SOLDIER_FIRE_LOG("SKIP (reloading)");
			return;
		}
		if (Ranged->GetCurrentAmmo() <= 0)
		{
			PD_SOLDIER_FIRE_LOG("RELOAD (ammo=0)");
			Ranged->Reload();
			return;
		}
	}

	// 우군 사선 안전망 — BT 외 자율 발사 경로에서도 프렌들리 파이어 차단. 타이머는 유지(우군 이동 시 즉시 재개).
	if (Combat->IsFriendlyInLineOfFire())
	{
		PD_SOLDIER_FIRE_LOG("SKIP (friendly in LOF)");
		return;
	}

	PD_SOLDIER_FIRE_LOG("FIRE");
	EquippedWeapon->Fire();
}
