#include "Component/PDVisionComponent.h"

#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "AttributeSet/PDAttributeSet.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/PDDetectable.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Characters/PDPlayerCharacter.h"

UPDVisionComponent::UPDVisionComponent()
{
	PrimaryComponentTick.bCanEverTick=true;
	PrimaryComponentTick.bStartWithTickEnabled=true;
}


void UPDVisionComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();


	if (!Owner)
	{
		return;
	}

	LastLocation=Owner->GetActorLocation();
	LastYaw=Owner->GetActorRotation().Yaw;
	SmoothedForward=Owner->GetActorForwardVector();

	UpdateFogOfWarMPC_Transform(0.f);
	UpdateFogOfWarMPC_Vision();
	ScheduleNextUpdate(UpdateInterval);
}

void UPDVisionComponent::EndPlay(EEndPlayReason::Type Reason)
{

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle);
	}
	for (AActor* Actor:VisibleActors)
	{
		if (IsValid(Actor)) UpdateExposure(Actor, 0.f);
	}
	VisibleActors.Empty();
	Super::EndPlay(Reason);
}

void UPDVisionComponent::BindToAttributeSet(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}


	ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetVisionRangeAttribute())
		.AddUObject(this, &UPDVisionComponent::OnVisionRangeChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetVisionAngleAttribute())
		.AddUObject(this, &UPDVisionComponent::OnVisionAngleChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetVisionUpdateIntervalAttribute())
		.AddUObject(this, &UPDVisionComponent::OnVisionUpdateIntervalChanged);

	if (const UPDAttributeSet* AS = ASC->GetSet<UPDAttributeSet>())
	{
		if (AS->GetVisionRange() > 0.f)
			VisionRange = AS->GetVisionRange();
		if (AS->GetVisionAngle() > 0.f)
			VisionAngle = AS->GetVisionAngle();
		if (AS->GetVisionUpdateInterval() > 0.f)
			UpdateInterval = AS->GetVisionUpdateInterval();


		UpdateFogOfWarMPC_Vision();
	}
	else
	{
	}
}

void UPDVisionComponent::SetVisionAngleOverride(float Angle)
{
	bHasAngleOverride=true;
	VisionAngleOverride=Angle;
}

void UPDVisionComponent::ClearVisionAngleOverride()
{
	bHasAngleOverride=false;
}

void UPDVisionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateFogOfWarMPC_Transform(DeltaTime);
}

void UPDVisionComponent::OnVisionRangeChanged(const FOnAttributeChangeData& Data)
{
	VisionRange=Data.NewValue;
	UpdateFogOfWarMPC_Vision();
}

void UPDVisionComponent::OnVisionAngleChanged(const FOnAttributeChangeData& Data)
{
	VisionAngle=Data.NewValue;
	UpdateFogOfWarMPC_Vision();
}

void UPDVisionComponent::OnVisionUpdateIntervalChanged(const FOnAttributeChangeData& Data)
{
	UpdateInterval=Data.NewValue;
}

void UPDVisionComponent::PerformVisionUpdate()
{
	AActor* Owner=GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector CurrentLocation=Owner->GetActorLocation();
	const float CurrentYaw=Owner->GetActorRotation().Yaw;

	const bool bMoved=FVector::Dist(CurrentLocation, LastLocation)>LocationThreshold;
	const bool bRotated=FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentYaw, LastYaw))>YawThreshold;
	const bool bMoving=bMoved||bRotated;

	if (bMoving)
	{
		LastLocation=CurrentLocation;
		LastYaw=CurrentYaw;
	}

	ScheduleNextUpdate(bMoving ? UpdateInterval : ThrottledInterval);

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	const float EffectiveRange = VisionRange * StaminaScale;

	World->OverlapMultiByChannel(Overlaps, CurrentLocation, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeSphere(EffectiveRange), Params);

	TSet<AActor*> NewVisible;
	for (const FOverlapResult& Result:Overlaps)
	{
		AActor* Target=Result.GetActor();
		if (!IsValid(Target)) continue;
		if (!Target->Implements<UPDDetectable>()) continue;
		if (!IsInCone(Target)) continue;
		if (!HasLineOfSight(Target)) continue;
		NewVisible.Add(Target);
	}
	for (AActor* Actor:VisibleActors)
	{
		if (!NewVisible.Contains(Actor)) UpdateExposure(Actor, 0.f);
	}
	for (AActor* Actor:NewVisible)
	{
		UpdateExposure(Actor, CalcExposure(Actor));
	}

	VisibleActors=MoveTemp(NewVisible);
	UpdateFogOfWarMPC_Vision();
}

float UPDVisionComponent::CalcExposure(AActor* Target) const
{
	const float EffectiveRange = VisionRange * StaminaScale;
	const float Distance=FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation());
	const float Linear=FMath::Clamp(1.f-(Distance/EffectiveRange), 0.f, 1.f);
	return Linear*Linear;
}


bool UPDVisionComponent::IsInCone(AActor* Target) const
{
	const FVector ToTargetRaw=Target->GetActorLocation()-GetOwner()->GetActorLocation();

	if (ToTargetRaw.SizeSquared()<FMath::Square(ProximityRadius)) return true;

	const APDPlayerCharacter* PlayerOwner = Cast<APDPlayerCharacter>(GetOwner());
	const FVector Forward = PlayerOwner
		? PlayerOwner->GetSharedVisionForwardVector(GetOwner()->GetActorLocation())
		: GetOwner()->GetActorForwardVector();
	const float Dot=FVector::DotProduct(Forward, ToTargetRaw.GetSafeNormal());
	return Dot>=FMath::Cos(FMath::DegreesToRadians(GetEffectiveAngle()*0.5f));
}

bool UPDVisionComponent::HasLineOfSight(AActor* Target) const
{
	FHitResult Hit;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	Params.AddIgnoredActor(Target);

	const bool bBlocked=GetWorld()->LineTraceSingleByChannel(Hit, GetOwner()->GetActorLocation(),
		Target->GetActorLocation(), ECC_GameTraceChannel1, Params);
	return !bBlocked;
}

void UPDVisionComponent::UpdateExposure(AActor* Target, float Exposure)
{
	IPDDetectable::Execute_OnVisionExposureChanged(Target, GetOwner(), Exposure);
}

void UPDVisionComponent::ScheduleNextUpdate(float Interval)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float SafeInterval = FMath::Max(Interval, 0.01f);
	World->GetTimerManager().SetTimer(TimerHandle, this,
		&UPDVisionComponent::PerformVisionUpdate, SafeInterval, false);

}

float UPDVisionComponent::GetEffectiveAngle() const
{
	return bHasAngleOverride?VisionAngleOverride : VisionAngle;
}

void UPDVisionComponent::UpdateStaminaScale(float Scale)
{
	StaminaScale=FMath::Clamp(Scale, 0.3f, 1.f);
	UpdateFogOfWarMPC_Vision();
}

void UPDVisionComponent::UpdateFogOfWarMPC_Transform(float DeltaTime)
{
	if (!FogOfWarMPC)
	{
		return;
	}
	AActor* Owner=GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	const FVector Loc=Owner->GetActorLocation();
	const APDPlayerCharacter* PlayerOwner = Cast<APDPlayerCharacter>(Owner);
	const FVector RawForward = PlayerOwner
		? PlayerOwner->GetSharedVisionForwardVector(Loc)
		: Owner->GetActorForwardVector();

	SmoothedForward=FMath::VInterpTo(SmoothedForward, RawForward, DeltaTime, ForwardSmoothSpeed);

	UpdateSharedFogOfWarMPC(DeltaTime);
}

void UPDVisionComponent::UpdateFogOfWarMPC_Vision()
{
	if (!FogOfWarMPC)
	{
		return;
	}

	UpdateSharedFogOfWarMPC(0.f);
}

void UPDVisionComponent::UpdateSharedFogOfWarMPC(float DeltaTime)
{
	if (!FogOfWarMPC || !GetWorld()) return;

	const int32 WritableSourceCount = FMath::Min(MaxSharedVisionSources, 2);

	TArray<UPDVisionComponent*> Sources;
	Sources.Reserve(WritableSourceCount);

	APawn* LocalPawn = nullptr;
	if (const APlayerController* LocalPC = GetWorld()->GetFirstPlayerController())
	{
		LocalPawn = LocalPC->GetPawn();
	}

	if (APDPlayerCharacter* LocalCharacter = Cast<APDPlayerCharacter>(LocalPawn))
	{
		if (UPDVisionComponent* LocalVision = LocalCharacter->FindComponentByClass<UPDVisionComponent>())
		{
			Sources.Add(LocalVision);
		}
		else
		{
		}
	}
	else
	{
	}

	for (TActorIterator<APDPlayerCharacter> It(GetWorld()); It; ++It)
	{
		if (Sources.Num() >= WritableSourceCount)
		{
			break;
		}

		APDPlayerCharacter* Character = *It;
		if (!IsValid(Character) || Character == LocalPawn)
		{
			continue;
		}

		if (UPDVisionComponent* VisionComponent = Character->FindComponentByClass<UPDVisionComponent>())
		{
			Sources.Add(VisionComponent);
		}
	}

	const int32 SourceCount = FMath::Min(Sources.Num(), WritableSourceCount);
	UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), FogOfWarMPC, TEXT("VisionSourceCount"), SourceCount);

	for (int32 Index = 0; Index < WritableSourceCount; ++Index)
	{
		if (!Sources.IsValidIndex(Index) || !IsValid(Sources[Index]) || !IsValid(Sources[Index]->GetOwner()))
		{
			WriteVisionSourceToMPC(Index, FVector::ZeroVector, FVector::ForwardVector, 0.f, 1.f, 0.f);
			continue;
		}

		UPDVisionComponent* Source = Sources[Index];
		AActor* SourceOwner = Source->GetOwner();
		const FVector SourceLocation = SourceOwner->GetActorLocation();
		const APDPlayerCharacter* PlayerOwner = Cast<APDPlayerCharacter>(SourceOwner);
		const FVector RawForward = PlayerOwner
			? PlayerOwner->GetSharedVisionForwardVector(SourceLocation)
			: SourceOwner->GetActorForwardVector();
		Source->SmoothedForward = FMath::VInterpTo(Source->SmoothedForward, RawForward, DeltaTime, Source->ForwardSmoothSpeed);

		const float AngleCos = FMath::Cos(FMath::DegreesToRadians(Source->GetEffectiveAngle() * 0.5f));
		WriteVisionSourceToMPC(
			Index,
			SourceLocation,
			Source->SmoothedForward,
			Source->VisionRange * Source->StaminaScale,
			AngleCos,
			Source->ProximityRadius);
	}
}

void UPDVisionComponent::WriteVisionSourceToMPC(int32 SourceIndex, const FVector& Location, const FVector& Forward, float Range, float AngleCos, float InProximityRadius) const
{
	if (!FogOfWarMPC) return;

	const FString Prefix = FString::Printf(TEXT("Vision%d"), SourceIndex);
	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), FogOfWarMPC,
		FName(*(Prefix + TEXT("WorldPosition"))), FLinearColor(Location.X, Location.Y, Location.Z, 0.f));
	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), FogOfWarMPC,
		FName(*(Prefix + TEXT("ForwardVector"))), FLinearColor(Forward.X, Forward.Y, Forward.Z, 0.f));
	UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), FogOfWarMPC,
		FName(*(Prefix + TEXT("VisionRange"))), Range);
	UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), FogOfWarMPC,
		FName(*(Prefix + TEXT("AngleCos"))), AngleCos);
	UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), FogOfWarMPC,
		FName(*(Prefix + TEXT("ProximityRadius"))), InProximityRadius);
}
