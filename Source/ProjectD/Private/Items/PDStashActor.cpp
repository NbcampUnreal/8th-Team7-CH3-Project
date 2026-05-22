#include "Items/PDStashActor.h"

#include "Components/BoxComponent.h"
#include "Component/PDInteractionOutlineComponent.h"
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

	OutlineComponent = CreateDefaultSubobject<UPDInteractionOutlineComponent>(TEXT("OutlineComponent"));
	OutlineComponent->SetupTrigger(InteractionCollision);

	CurrentDoorAngle = ClosedDoorAngle;
	TargetDoorAngle = ClosedDoorAngle;
}

void APDStashActor::BeginPlay()
{
	Super::BeginPlay();

	ConfigureInteractionCollision();
	if (OutlineComponent)
	{
		OutlineComponent->SetupTrigger(InteractionCollision);
	}
	SetDoorOpen(false, true);
}

void APDStashActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ConfigureInteractionCollision();
	CurrentDoorAngle = ClosedDoorAngle;
	TargetDoorAngle = ClosedDoorAngle;
}

void APDStashActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindContainerClose();

	Super::EndPlay(EndPlayReason);
}

void APDStashActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CurrentDoorAngle = FMath::FInterpTo(CurrentDoorAngle, TargetDoorAngle, DeltaSeconds, DoorInterpSpeed);

	if (FMath::IsNearlyEqual(CurrentDoorAngle, TargetDoorAngle, 0.1f))
	{
		CurrentDoorAngle = TargetDoorAngle;
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
		BindContainerClose(PlayerController);
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
}

void APDStashActor::BindContainerClose(APDPlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	UnbindContainerClose();
	BoundPlayerController = PlayerController;
	PlayerController->OnStashInterfaceClosed.AddUObject(this, &APDStashActor::HandleContainerClosed);
}

void APDStashActor::UnbindContainerClose()
{
	if (BoundPlayerController.IsValid())
	{
		BoundPlayerController->OnStashInterfaceClosed.RemoveAll(this);
	}

	BoundPlayerController.Reset();
}

void APDStashActor::HandleContainerClosed(UPDStashComponent* ClosedStashComponent)
{
	if (ClosedStashComponent != StashComponent)
	{
		return;
	}

	UnbindContainerClose();
	OnStorageClosed.Broadcast(this);
}
