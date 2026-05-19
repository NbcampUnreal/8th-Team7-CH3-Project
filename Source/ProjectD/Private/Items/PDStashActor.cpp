#include "Items/PDStashActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Items/PDStashComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

APDStashActor::APDStashActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	InteractionCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionCollision"));
	SetRootComponent(InteractionCollision);
	InteractionCollision->SetBoxExtent(FVector(80.f, 80.f, 80.f));
	ConfigureInteractionCollision();


	StashComponent = CreateDefaultSubobject<UPDStashComponent>(TEXT("StashComponent"));

	CurrentDoorAngle = ClosedDoorAngle;
	TargetDoorAngle = ClosedDoorAngle;
}

void APDStashActor::BeginPlay()
{
	Super::BeginPlay();

	ConfigureInteractionCollision();
	CacheLinkedDoorComponents(true);
	SetDoorOpen(false, true);
}

void APDStashActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ConfigureInteractionCollision();
	CacheLinkedDoorComponents(true);
	CurrentDoorAngle = ClosedDoorAngle;
	TargetDoorAngle = ClosedDoorAngle;
	ApplyDoorAngle(CurrentDoorAngle);
}

void APDStashActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindStashClose();

	Super::EndPlay(EndPlayReason);
}

void APDStashActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CurrentDoorAngle = FMath::FInterpTo(CurrentDoorAngle, TargetDoorAngle, DeltaSeconds, DoorInterpSpeed);
	ApplyDoorAngle(CurrentDoorAngle);

	if (FMath::IsNearlyEqual(CurrentDoorAngle, TargetDoorAngle, 0.1f))
	{
		CurrentDoorAngle = TargetDoorAngle;
		ApplyDoorAngle(CurrentDoorAngle);
		SetActorTickEnabled(false);
	}

}

void APDStashActor::Interact_Implementation(AActor* Interactor)
{
	APawn* InteractingPawn = Cast<APawn>(Interactor);
	if (!InteractingPawn)
	{
		return;
	}

	APDPlayerController* PlayerController = Cast<APDPlayerController>(InteractingPawn->GetController());
	if (!PlayerController)
	{
		return;
	}

	if (PlayerController->IsStashInterfaceOpen() && PlayerController->GetActiveStashComponent() == StashComponent)
	{
		PlayerController->CloseStashInterface();
		return;
	}

	PlayerController->OpenStashInterface(StashComponent);

	if (PlayerController->IsStashInterfaceOpen() && PlayerController->GetActiveStashComponent() == StashComponent)
	{
		BindStashClose(PlayerController);
		OnStorageOpened.Broadcast(this);
	}
}

void APDStashActor::ConfigureInteractionCollision() const
{
	if (!InteractionCollision)
	{
		return;
	}

	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollision->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionCollision->SetGenerateOverlapEvents(true);
}

void APDStashActor::SetDoorOpen(bool bOpen, bool bInstant)
{
	const float NewTargetDoorAngle = bOpen ? OpenDoorAngle : ClosedDoorAngle;
	const bool bTargetChanged = !FMath::IsNearlyEqual(TargetDoorAngle, NewTargetDoorAngle, 0.1f);
	TargetDoorAngle = NewTargetDoorAngle;

	if (bTargetChanged && !bInstant)
	{
		PlayDoorSound(bOpen);
	}

	if (bInstant)
	{
		CurrentDoorAngle = TargetDoorAngle;
		ApplyDoorAngle(CurrentDoorAngle);
		SetActorTickEnabled(false);
		return;
	}

	SetActorTickEnabled(true);
}

void APDStashActor::PlayDoorSound(bool bOpen) const
{
	USoundBase* Sound = bOpen ? OpenSound.Get() : CloseSound.Get();

	if (!Sound)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation(), SoundVolumeMultiplier, SoundPitchMultiplier);
}

void APDStashActor::ApplyDoorAngle(float Angle)
{
	CacheLinkedDoorComponents(false);

	const float OpenAlpha = GetDoorOpenAlpha(Angle);

	if (CachedOverDoorComponent.IsValid())
	{
		CachedOverDoorComponent->SetRelativeLocation(FMath::Lerp(OverDoorClosedLocation, OverDoorClosedLocation + OverDoorOpenOffset, OpenAlpha));
	}

	if (CachedUnderDoorComponent.IsValid())
	{
		CachedUnderDoorComponent->SetRelativeLocation(FMath::Lerp(UnderDoorClosedLocation, UnderDoorClosedLocation + UnderDoorOpenOffset, OpenAlpha));
	}
}

void APDStashActor::CacheLinkedDoorComponents(bool bResetClosedLocations)
{
	if (!LinkedStorageDoor)
	{
		CachedOverDoorComponent.Reset();
		CachedUnderDoorComponent.Reset();
		return;
	}

	if (!CachedOverDoorComponent.IsValid())
	{
		CachedOverDoorComponent = FindLinkedDoorComponent(OverDoorComponentName);
	}

	if (!CachedUnderDoorComponent.IsValid())
	{
		CachedUnderDoorComponent = FindLinkedDoorComponent(UnderDoorComponentName);
	}

	if (bResetClosedLocations)
	{
		if (CachedOverDoorComponent.IsValid())
		{
			OverDoorClosedLocation = CachedOverDoorComponent->GetRelativeLocation();
		}

		if (CachedUnderDoorComponent.IsValid())
		{
			UnderDoorClosedLocation = CachedUnderDoorComponent->GetRelativeLocation();
		}
	}
}

USceneComponent* APDStashActor::FindLinkedDoorComponent(FName ComponentName) const
{
	if (!LinkedStorageDoor || ComponentName.IsNone())
	{
		return nullptr;
	}

	TArray<USceneComponent*> SceneComponents;
	LinkedStorageDoor->GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent && SceneComponent->GetFName() == ComponentName)
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

float APDStashActor::GetDoorOpenAlpha(float Angle) const
{
	const float Denominator = OpenDoorAngle - ClosedDoorAngle;
	if (FMath::IsNearlyZero(Denominator))
	{
		return 0.f;
	}

	return FMath::Clamp((Angle - ClosedDoorAngle) / Denominator, 0.f, 1.f);
}

void APDStashActor::BindStashClose(APDPlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	UnbindStashClose();
	BoundPlayerController = PlayerController;
	PlayerController->OnStashInterfaceClosed.AddUObject(this, &APDStashActor::HandleStashInterfaceClosed);
}

void APDStashActor::UnbindStashClose()
{
	if (BoundPlayerController.IsValid())
	{
		BoundPlayerController->OnStashInterfaceClosed.RemoveAll(this);
	}

	BoundPlayerController.Reset();
}

void APDStashActor::HandleStashInterfaceClosed(UPDStashComponent* ClosedStashComponent)
{
	if (ClosedStashComponent != StashComponent)
	{
		return;
	}

	UnbindStashClose();
	OnStorageClosed.Broadcast(this);
}
