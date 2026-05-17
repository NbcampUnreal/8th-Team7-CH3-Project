#include "Characters/Base/PDEnemyBase.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "AttributeSet/PDAttributeSet.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Items/PDItemBase.h"
#include "Items/PDLootItem.h"
#include "Enemy/Components/PDCombatComponent.h"
#include "Component/PDWeaponComponent.h"
#include "Data/PDQuestComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "Widgets/Enemy/PDEnemyOverheadWidget.h"

APDEnemyBase::APDEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// 적 AI 라 Minimal 로 충분. (큰 데이터는 호스트만 보유)
	if (ASC)
	{
		ASC->SetIsReplicated(true);
		ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	}

	// Hostile 기본값. 자식(예: PDSoldier)에서 재정의 가능.
	TeamID = 2;

	// 머리 위 HUD — Screen 공간 위젯. 월드 좌표를 화면에 투영해 2D 오버레이로 그리므로 World 대비 비용이 낮음(머터리얼·렌더타깃·뎁스 처리 없음).
	// 주의: 화면공간이라 항상 카메라를 향하고 오클루전이 적용되지 않으며, 로컬 PlayerController 기준으로만 렌더된다.
	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidgetComponent->SetupAttachment(GetRootComponent());
	OverheadWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	OverheadWidgetComponent->SetDrawAtDesiredSize(true);
	OverheadWidgetComponent->SetPivot(FVector2D(0.5f, 1.f));
	OverheadWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
	OverheadWidgetComponent->SetGenerateOverlapEvents(false);
	OverheadWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APDEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	if (UPDCombatComponent* Combat = FindComponentByClass<UPDCombatComponent>())
		Combat->OnTargetChanged.AddDynamic(this, &APDEnemyBase::OnCombatTargetChanged);

	if (USkeletalMeshComponent* SkelMesh=GetMesh())
	{
		for (int32 i=0; i<SkelMesh->GetNumMaterials(); i++)
		{
			UMaterialInstanceDynamic* DynMat=SkelMesh->CreateAndSetMaterialInstanceDynamic(i);
			if (DynMat) DitherMaterials.Add(DynMat);
		}
	}

	// 오버헤드 HUD 초기화 — 위젯 클래스 미지정 시 캐시는 nullptr 로 남아 모든 Show* 호출이 no-op.
	if (OverheadWidgetComponent)
	{
		if (OverheadWidgetClass)
		{
			OverheadWidgetComponent->SetWidgetClass(OverheadWidgetClass);
		}
		OverheadWidgetComponent->InitWidget();
		OverheadWidget = Cast<UPDEnemyOverheadWidget>(OverheadWidgetComponent->GetUserWidgetObject());
	}

	// Torso HP 변경 구독 + 초기값 push. ShowHealth() 전까지 위젯은 collapsed 라 첫 push 는 백그라운드 갱신.
	if (ASC && AttributeSet)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetTorsoHPAttribute())
			.AddUObject(this, &APDEnemyBase::OnTorsoHPChanged);

		if (OverheadWidget)
		{
			OverheadWidget->SetHealth(AttributeSet->GetTorsoHP(), AttributeSet->GetMaxTorsoHP());
		}
	}
}

void APDEnemyBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickFootstep(DeltaSeconds);
}

void APDEnemyBase::TickFootstep(float DeltaSeconds)
{
	if (!FootstepSound || CurrentState == EPDEnemyState::Dead) return;

	const UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp || !MoveComp->IsMovingOnGround()) return;

	const float Speed = GetVelocity().Size2D();
	if (Speed < FootstepMinSpeed)
	{
		// 정지 중엔 누적값 리셋 — 다음 출발 직후 즉시 발자국 폭주 방지.
		FootstepDistanceAccumulator = 0.f;
		return;
	}

	FootstepDistanceAccumulator += Speed * DeltaSeconds;
	if (FootstepDistanceAccumulator < FootstepStrideDistance) return;

	FootstepDistanceAccumulator = 0.f;

	const FVector Loc = GetActorLocation();
	UGameplayStatics::PlaySoundAtLocation(
		this, FootstepSound, Loc,
		1.f, 1.f, 0.f, SoundAttenuation);

	// AI 청각용 노이즈 — 0 이면 스킵.
	if (FootstepNoiseRange > 0.f)
	{
		MakeNoise(1.f, this, Loc, FootstepNoiseRange);
	}
}

void APDEnemyBase::OnTorsoHPChanged(const FOnAttributeChangeData& Data)
{
	if (!OverheadWidget || !AttributeSet) return;
	OverheadWidget->SetHealth(Data.NewValue, AttributeSet->GetMaxTorsoHP());
}


uint8 APDEnemyBase::GetTeamID_Implementation() const
{
	return TeamID;
}

EPDStaminaStatus APDEnemyBase::GetStaminaStatus_Implementation() const
{
	// 비-Biped 디폴트. Biped 자식 클래스가 AttributeSet 기반으로 실제 상태 반환.
	return EPDStaminaStatus::None;
}

void APDEnemyBase::SetEnemyState(EPDEnemyState NewState)
{
	if (CurrentState == NewState) return;

	CurrentState = NewState;

	switch (NewState)
	{
	case EPDEnemyState::Idle:   OnEnterState_Idle();   break;
	case EPDEnemyState::Alert:  OnEnterState_Alert();  break;
	case EPDEnemyState::Chase:  OnEnterState_Chase();  break;
	case EPDEnemyState::Combat: OnEnterState_Combat(); break;
	case EPDEnemyState::Dead:   OnEnterState_Dead();   break;
	default: break;
	}

	OnEnemyStateChanged(NewState);

	// Dead 는 위젯 전체 숨김(시체 위 HP 잔존 방지). 그 외엔 상태 말풍선 갱신.
	if (OverheadWidget)
	{
		if (NewState == EPDEnemyState::Dead)
		{
			OverheadWidget->HideAll();
		}
		else
		{
			OverheadWidget->ShowSpeech(NewState, SpeechBubbleDuration);
		}
	}
}

void APDEnemyBase::ApplyDamage_Implementation(const FPDDamageInfo& DamageInfo)
{
	Super::ApplyDamage_Implementation(DamageInfo);

	// 피격음: 살아있을 때만 재생. 사망 데미지는 OnEnterState_Dead 의 DeathSound 로 일원화.
	if (HurtSound && IsAlive_Implementation())
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, HurtSound, GetActorLocation(),
			1.f, 1.f, 0.f, SoundAttenuation);
	}

	// 첫 피격 시 헬스바 영구 노출. 이후 호출은 플래그로 단락.
	if (!bHealthBarShown && OverheadWidget)
	{
		bHealthBarShown = true;
		OverheadWidget->ShowHealth();
	}

	// Combat/Dead는 BT가 이미 능동 대응 중이거나 종료된 상태 — 추적 신호 무시.
	if (CurrentState == EPDEnemyState::Combat || CurrentState == EPDEnemyState::Dead) return;
	if (!IsAlive_Implementation()) return;

	AActor* InstigatorActor = DamageInfo.Instigator.Get();
	if (!InstigatorActor) return;

	// Controller가 Instigator로 들어온 경우 Pawn으로 보정 (위치 신뢰성 확보).
	if (AController* InstController = Cast<AController>(InstigatorActor))
	{
		InstigatorActor = InstController->GetPawn();
		if (!InstigatorActor) return;
	}

	// 같은 팀이 가한 데미지는 추적 트리거 무시 (오발/AOE 등).
	if (const IGenericTeamAgentInterface* InstTeam = Cast<IGenericTeamAgentInterface>(InstigatorActor))
	{
		if (InstTeam->GetGenericTeamId() == GetGenericTeamId()) return;
	}

	const FVector HintLocation = InstigatorActor->GetActorLocation();

	if (UPDCombatComponent* Combat = FindComponentByClass<UPDCombatComponent>())
	{
		Combat->SetLastNoiseLocation(InstigatorActor, HintLocation);
	}

	// BB 직접 갱신 — AIController는 OnNoiseHintChanged를 구독하지 않으므로 HandleNoiseHeard와 동일 패턴으로 채운다.
	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AICon->GetBlackboardComponent())
		{
			BB->SetValueAsBool  (PDBTKeys::HasNoiseHint,      true);
			BB->SetValueAsVector(PDBTKeys::LastNoiseLocation, HintLocation);
		}
	}
}

void APDEnemyBase::OnEnterState_Dead()
{
	// 충돌/이동 정리. 시체가 발사체를 막지 않도록. (StimuliSource 해제는 베이스 HandleDeath 에서.)
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}

	// 사망 시 드랍 + 시체 컨테이너. 디자이너가 BP 에서 추가 VFX/사운드는 OnLootDropped 로 확장.
	DropLootOnDeath();
	SpawnCorpseContainer();

	// 사망 모션 재생. 애니메이션이 없는 경우에도 문제 없도록 null 체크.
	if(DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}

	// 사망음: SetLifeSpan 으로 액터가 곧 소멸하므로 PlaySoundAtLocation 으로 detach 재생.
	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, DeathSound, GetActorLocation(),
			1.f, 1.f, 0.f, SoundAttenuation);
	}

	// 0 이하면 영구 보존(시체 컨테이너로 대체되는 시나리오 등) — 그 외에는 LifeSpan 예약.
	if (CorpseDespawnDelay > 0.f)
	{
		SetLifeSpan(CorpseDespawnDelay);
	}
}

void APDEnemyBase::HandleDeath(AActor* Killer)
{
	SetEnemyState(EPDEnemyState::Dead);

	AActor* QuestOwner = Killer;
	if (AController* KillerController = Cast<AController>(Killer))
	{
		QuestOwner = KillerController->GetPawn();
	}

	if (QuestOwner)
	{
		if (UPDQuestComponent* QuestComponent = QuestOwner->FindComponentByClass<UPDQuestComponent>())
		{
			const FName EnemyID = !QuestEnemyID.IsNone() ? QuestEnemyID : GetFName();
			QuestComponent->ReportEnemyKilled(EnemyID, 1);
			if (bIsQuestEnemy)
			{
				QuestComponent->ReportQuestEnemyKilled(EnemyID, 1);
			}
		}
	}

	Super::HandleDeath(Killer);
}

void APDEnemyBase::OnVisionExposureChanged_Implementation(AActor* Observer, float Exposure)
{
	for (UMaterialInstanceDynamic* Mat : DitherMaterials)
	{
		if (Mat) Mat->SetScalarParameterValue(TEXT("Exposure"), Exposure);
	}
}

void APDEnemyBase::DropLootOnDeath()
{
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<AActor*> Spawned;
	const FVector Origin = GetActorLocation();

	for (const FPDLootEntry& Entry : LootTable)
	{
		if (!Entry.ItemClass) continue;
		if (FMath::FRand() > Entry.DropChance) continue;

		const int32 MinQ = FMath::Max(1, Entry.MinQuantity);
		const int32 MaxQ = FMath::Max(MinQ, Entry.MaxQuantity);
		const int32 Quantity = FMath::RandRange(MinQ, MaxQ);

		const FVector Offset = (LootSpawnRadius > 0.f)
			? FMath::VRand() * FMath::FRandRange(0.f, LootSpawnRadius)
			: FVector::ZeroVector;

		// Deferred 스폰: BeginPlay(=LoadItemData) 전에 ItemID 와 Quantity 를 주입.
		const FTransform SpawnXform(FRotator::ZeroRotator, Origin + FVector(Offset.X, Offset.Y, 0.f));

		APDLootItem* Item = World->SpawnActorDeferred<APDLootItem>(
			Entry.ItemClass,
			SpawnXform,
			/*Owner=*/this,
			/*Instigator=*/nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

		if (Item)
		{
			if (!Entry.ItemID.IsNone())
			{
				Item->SetItemID(Entry.ItemID);
			}
			Item->Quantity = Quantity;
			Item->FinishSpawning(SpawnXform);
			Spawned.Add(Item);
		}
	}

	if (Spawned.Num() > 0)
	{
		OnLootDropped(Spawned);
	}
}

AActor* APDEnemyBase::SpawnCorpseContainer()
{
	if (!CorpseContainerClass) return nullptr;

	UWorld* World = GetWorld();
	if (!World) return nullptr;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = this;

	AActor* Spawned = World->SpawnActor<AActor>(
		CorpseContainerClass,
		GetActorLocation(),
		GetActorRotation(),
		Params);

	CorpseContainerInstance = Spawned;
	return Spawned;
}

void APDEnemyBase::SetAimTarget(AActor* Target)
{
	if (WeaponComponent)
		WeaponComponent->SetAimTarget(Target);
}

void APDEnemyBase::ClearAimTarget()
{
	if (WeaponComponent)
		WeaponComponent->ClearAimTarget();
}

void APDEnemyBase::OnCombatTargetChanged(AActor* NewTarget)
{
	if (NewTarget) SetAimTarget(NewTarget);
	else           ClearAimTarget();
}