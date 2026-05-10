#include "Characters/Base/PDEnemyBase.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Items/PDItemBase.h"

APDEnemyBase::APDEnemyBase()
{
	// 적 AI 라 Minimal 로 충분. (큰 데이터는 호스트만 보유)
	if (ASC)
	{
		ASC->SetIsReplicated(true);
		ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	}

	// Hostile 기본값. 자식(예: PDSoldier)에서 재정의 가능.
	TeamID = 2;
}

uint8 APDEnemyBase::GetTeamID_Implementation() const
{
	return TeamID;
}

EPDStaminaStatus APDEnemyBase::GetStaminaStatus_Implementation() const
{
	// 비-Biped 디폴트. Biped 자식 클래스가 StatComponent로부터 실제 상태 반환.
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
}

void APDEnemyBase::HandleDeath(AActor* Killer)
{
	SetEnemyState(EPDEnemyState::Dead);
	Super::HandleDeath(Killer);
}

void APDEnemyBase::OnVisionExposureChanged_Implementation(AActor* Observer, float Exposure)
{
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

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Params.Owner = this;

		APDItemBase* Item = World->SpawnActor<APDItemBase>(
			Entry.ItemClass,
			Origin + FVector(Offset.X, Offset.Y, 0.f),
			FRotator::ZeroRotator,
			Params);

		if (Item)
		{
			Item->Quantity = Quantity;
			Spawned.Add(Item);
		}
	}

	if (Spawned.Num() > 0)
	{
		OnLootDropped(Spawned);
	}
}

void APDEnemyBase::SpawnCorpseContainer()
{
	if (!CorpseContainerClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = this;

	World->SpawnActor<AActor>(
		CorpseContainerClass,
		GetActorLocation(),
		GetActorRotation(),
		Params);
}
