#include "Ability/GA_BombingAbility.h"

#include "Ability/TargetActors/PDGroundTargetActor.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Components/PrimitiveComponent.h"
#include "Core/PDPlayerController.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Interfaces/PDDamageable.h"
#include "TimerManager.h"

UGA_BombingAbility::UGA_BombingAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	TargetActorClass = APDGroundTargetActor::StaticClass();
	SetAssetTags(FGameplayTagContainer(PDGameplayTags::Input_Bombing));
}

void UGA_BombingAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (TargetActorClass)
	{
		StartTargeting();
		return;
	}

	FVector TargetLocation = FVector::ZeroVector;
	FVector TargetNormal = FVector::UpVector;
	if (!ResolveFallbackTarget(TargetLocation, TargetNormal))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ConfirmBombingLocation(TargetLocation, TargetNormal);
}

void UGA_BombingAbility::StartTargeting()
{
	UAbilityTask_WaitTargetData* TargetTask = UAbilityTask_WaitTargetData::WaitTargetData(
		this,
		NAME_None,
		EGameplayTargetingConfirmation::Custom,
		TargetActorClass);
	if (!TargetTask)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	AGameplayAbilityTargetActor* SpawnedActor = nullptr;
	if (TargetTask->BeginSpawningActor(this, TargetActorClass, SpawnedActor))
	{
		if (APDGroundTargetActor* GroundTargetActor = Cast<APDGroundTargetActor>(SpawnedActor))
		{
			GroundTargetActor->SetTargetRadius(Radius);
			GroundTargetActor->SetMaxTargetRange(MaxTargetRange);
			GroundTargetActor->SetTraceChannel(TargetTraceChannel);
			GroundTargetActor->SetDecalMaterial(TargetDecalMaterial);
		}

		TargetTask->FinishSpawningActor(this, SpawnedActor);
	}

	TargetTask->ValidData.AddDynamic(this, &UGA_BombingAbility::OnTargetDataReady);
	TargetTask->Cancelled.AddDynamic(this, &UGA_BombingAbility::OnTargetDataCancelled);
	TargetTask->ReadyForActivation();
}

void UGA_BombingAbility::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetData)
{
	const FGameplayAbilityTargetData* Data = TargetData.Get(0);
	const FHitResult* HitResult = Data ? Data->GetHitResult() : nullptr;
	if (!HitResult)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	const FVector TargetLocation = HitResult->ImpactPoint.IsNearlyZero() ? HitResult->Location : HitResult->ImpactPoint;
	const FVector TargetNormal = HitResult->ImpactNormal.IsNearlyZero() ? FVector::UpVector : HitResult->ImpactNormal;
	ConfirmBombingLocation(TargetLocation, TargetNormal);
}

void UGA_BombingAbility::OnTargetDataCancelled(const FGameplayAbilityTargetDataHandle& TargetData)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_BombingAbility::ConfirmBombingLocation(const FVector& TargetLocation, const FVector& TargetNormal)
{
	APDCharacterBase* Character = GetPDCharacter();
	if (!Character)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (!Character->HasAuthority())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	if (ImpactDelay <= 0.f)
	{
		ExecuteBombingAtLocation(TargetLocation, TargetNormal);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(
			TimerHandle,
			FTimerDelegate::CreateUObject(this, &UGA_BombingAbility::ExecuteBombingAtLocation, TargetLocation, TargetNormal),
			ImpactDelay,
			false);
	}
}

void UGA_BombingAbility::ExecuteBombingAtLocation(FVector TargetLocation, FVector TargetNormal)
{
	APDCharacterBase* Character = GetPDCharacter();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!Character || !Character->HasAuthority() || !SourceASC)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	FGameplayCueParameters CueParams;
	CueParams.Location = TargetLocation;
	CueParams.Normal = TargetNormal;
	CueParams.RawMagnitude = Radius;
	CueParams.Instigator = Character;
	CueParams.EffectCauser = Character;
	SourceASC->ExecuteGameplayCue(PDGameplayTags::GameplayCue_Ability_Bombing, CueParams);

	ApplyAreaDamage(TargetLocation, TargetNormal);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_BombingAbility::ApplyAreaDamage(const FVector& TargetLocation, const FVector& TargetNormal)
{
	APDCharacterBase* Character = GetPDCharacter();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UWorld* World = GetWorld();
	if (!Character || !SourceASC || !World)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GA_BombingAbility), false);
	QueryParams.AddIgnoredActor(Character);

	World->OverlapMultiByObjectType(
		Overlaps,
		TargetLocation,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	TSet<AActor*> DamagedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!IsValid(HitActor) || DamagedActors.Contains(HitActor))
		{
			continue;
		}

		DamagedActors.Add(HitActor);
		const FHitResult DamageHit = MakeAreaDamageHit(HitActor, TargetLocation, TargetNormal);

		bool bAppliedDamageEffect = false;
		if (DamageEffectClass)
		{
			if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor))
			{
				FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
				Context.AddInstigator(Character, Character);
				Context.AddSourceObject(this);
				Context.AddHitResult(DamageHit);

				FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), Context);
				if (Spec.IsValid())
				{
					Spec.Data->SetSetByCallerMagnitude(PDGameplayTags::Data_Damage, Damage);
					SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
					bAppliedDamageEffect = true;
				}
			}
		}

		if (!bAppliedDamageEffect && bFallbackToDamageableInterface && HitActor->Implements<UPDDamageable>())
		{
			FPDDamageInfo DamageInfo;
			DamageInfo.BaseDamage = Damage;
			DamageInfo.Instigator = Character;
			DamageInfo.HitResult = DamageHit;
			IPDDamageable::Execute_ApplyDamage(HitActor, DamageInfo);
		}
	}
}

bool UGA_BombingAbility::ResolveFallbackTarget(FVector& OutLocation, FVector& OutNormal) const
{
	const APDCharacterBase* Character = GetPDCharacter();
	if (!Character)
	{
		return false;
	}

	if (const APDPlayerController* PDPC = Cast<APDPlayerController>(Character->GetController()))
	{
		FVector AimLocation = FVector::ZeroVector;
		if (PDPC->GetCachedAimWorldLocation(AimLocation))
		{
			OutLocation = AimLocation;
			OutNormal = FVector::UpVector;
			return true;
		}
	}

	OutLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * MaxTargetRange;
	OutNormal = FVector::UpVector;
	return true;
}

FHitResult UGA_BombingAbility::MakeAreaDamageHit(AActor* HitActor, const FVector& TargetLocation, const FVector& TargetNormal) const
{
	FVector HitLocation = HitActor ? HitActor->GetActorLocation() : TargetLocation;
	UPrimitiveComponent* HitComponent = HitActor ? Cast<UPrimitiveComponent>(HitActor->GetRootComponent()) : nullptr;
	if (HitComponent)
	{
		FVector ClosestPoint = FVector::ZeroVector;
		if (HitComponent->GetClosestPointOnCollision(TargetLocation, ClosestPoint) >= 0.f)
		{
			HitLocation = ClosestPoint;
		}
	}

	const FVector ImpactNormal = (HitLocation - TargetLocation).GetSafeNormal(UE_SMALL_NUMBER, TargetNormal);
	FHitResult Hit(HitActor, HitComponent, HitLocation, ImpactNormal);
	Hit.bBlockingHit = true;
	Hit.TraceStart = TargetLocation + FVector(0.f, 0.f, Radius);
	Hit.TraceEnd = HitLocation;
	Hit.Location = HitLocation;
	Hit.ImpactPoint = HitLocation;
	Hit.Normal = ImpactNormal;
	Hit.ImpactNormal = ImpactNormal;
	return Hit;
}
