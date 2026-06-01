#include "Ability/TargetActors/PDGroundTargetActor.h"

#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInterface.h"

APDGroundTargetActor::APDGroundTargetActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	bDestroyOnConfirmation = true;
	ShouldProduceTargetDataOnServer = false;

	TargetDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("TargetDecal"));
	RootComponent = TargetDecal;
	TargetDecal->SetHiddenInGame(true);
	TargetDecal->DecalSize = FVector(DecalDepth, TargetRadius, TargetRadius);
}

void APDGroundTargetActor::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);

	if (Ability && Ability->GetCurrentActorInfo())
	{
		CachedSourceActor = Ability->GetCurrentActorInfo()->AvatarActor.Get();
		CachedPlayerController = Ability->GetCurrentActorInfo()->PlayerController.Get();
	}

	if (DecalMaterial)
	{
		TargetDecal->SetDecalMaterial(DecalMaterial);
	}

	UpdateTargetHit();
	UpdateDecal();
}

void APDGroundTargetActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateTargetHit();
	UpdateDecal();

	if (APlayerController* PC = CachedPlayerController.Get())
	{
		if (bCancelOnEscape && PC->WasInputKeyJustPressed(EKeys::Escape))
		{
			CancelTargeting();
			return;
		}

		if (bCancelOnSecondaryClick && PC->WasInputKeyJustPressed(EKeys::RightMouseButton))
		{
			CancelTargeting();
			return;
		}

		if (bConfirmOnPrimaryClick && PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
		{
			ConfirmTargetingAndContinue();
		}
	}
}

bool APDGroundTargetActor::IsConfirmTargetingAllowed()
{
	return bHasCurrentHit && IsTargetInRange();
}

void APDGroundTargetActor::ConfirmTargetingAndContinue()
{
	if (!IsConfirmTargetingAllowed())
	{
		return;
	}

	FGameplayAbilityTargetDataHandle DataHandle;
	DataHandle.Add(new FGameplayAbilityTargetData_SingleTargetHit(CurrentHit));
	TargetDataReadyDelegate.Broadcast(DataHandle);
}

void APDGroundTargetActor::CancelTargeting()
{
	Super::CancelTargeting();
}

void APDGroundTargetActor::SetTargetRadius(float InRadius)
{
	TargetRadius = FMath::Max(1.f, InRadius);
	UpdateDecal();
}

void APDGroundTargetActor::SetMaxTargetRange(float InRange)
{
	MaxTargetRange = FMath::Max(0.f, InRange);
}

void APDGroundTargetActor::SetTraceChannel(TEnumAsByte<ECollisionChannel> InTraceChannel)
{
	TraceChannel = InTraceChannel;
}

void APDGroundTargetActor::SetDecalMaterial(UMaterialInterface* InMaterial)
{
	DecalMaterial = InMaterial;
	if (TargetDecal && DecalMaterial)
	{
		TargetDecal->SetDecalMaterial(DecalMaterial);
	}
}

bool APDGroundTargetActor::UpdateTargetHit()
{
	APlayerController* PC = CachedPlayerController.Get();
	if (!PC)
	{
		bHasCurrentHit = false;
		return false;
	}

	FHitResult Hit;
	if (!PC->GetHitResultUnderCursor(TraceChannel, true, Hit))
	{
		bHasCurrentHit = false;
		return false;
	}

	CurrentHit = Hit;
	bHasCurrentHit = true;
	return true;
}

void APDGroundTargetActor::UpdateDecal()
{
	if (!TargetDecal)
	{
		return;
	}

	TargetDecal->DecalSize = FVector(DecalDepth, TargetRadius, TargetRadius);
	TargetDecal->SetHiddenInGame(!bHasCurrentHit);

	if (!bHasCurrentHit)
	{
		return;
	}

	TargetDecal->SetWorldLocation(CurrentHit.ImpactPoint);
	TargetDecal->SetWorldRotation(CurrentHit.ImpactNormal.Rotation());
}

bool APDGroundTargetActor::IsTargetInRange() const
{
	if (MaxTargetRange <= 0.f)
	{
		return true;
	}

	const AActor* Source = CachedSourceActor.Get();
	if (!Source)
	{
		return true;
	}

	return FVector::DistSquared2D(Source->GetActorLocation(), CurrentHit.ImpactPoint) <= FMath::Square(MaxTargetRange);
}
