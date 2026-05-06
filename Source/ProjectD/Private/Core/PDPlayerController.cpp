#include "Core/PDPlayerController.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

#include "GameplayTag/PDGameplayTags.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Core/PDGameMode.h"
#include "Engine/LocalPlayer.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "Input/PDInputComponent.h"

APDPlayerController::APDPlayerController()
{
	PrimaryActorTick.bCanEverTick=true;
}

void APDPlayerController::RequestExtraction()
{
	if (APDGameMode* GM=GetWorld()->GetAuthGameMode<APDGameMode>())
	{
		GM->RequestExtraction(this);
	}
}

void APDPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	UpdateAimRotation();
}

void APDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem=
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	UPDInputComponent* PDIC=CastChecked<UPDInputComponent>(InputComponent);
	if (!InputConfig) return;

	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Move, 
		ETriggerEvent::Triggered, this, &APDPlayerController::OnMove);
	PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Jump,
	ETriggerEvent::Started, this, &APDPlayerController::OnJump);
	PDIC->BindAbilityActions(InputConfig, this, &APDPlayerController::OnAbilityInputPressed,
		&APDPlayerController::OnAbilityInputReleased);
}

void APDPlayerController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor=true;  

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false); 
	SetInputMode(InputMode);
}

void APDPlayerController::OnMove(const struct FInputActionValue& Value)
{
	APawn* ControlledPawn =GetPawn();
	if (!ControlledPawn) return;

	const FVector2D MoveInput=Value.Get<FVector2D>();
	ControlledPawn->AddMovementInput(FVector::ForwardVector, MoveInput.Y);
	ControlledPawn->AddMovementInput(FVector::RightVector, MoveInput.X);
}

void APDPlayerController::OnJump()
{
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetPawn()))
	{
		OwnerCharacter->Jump();
	}
}

void APDPlayerController::OnAbilityInputPressed(FGameplayTag InputTag)
{
	if (UAbilitySystemComponent* ASC=UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(InputTag));
	}
}


void APDPlayerController::OnAbilityInputReleased(FGameplayTag InputTag)
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
	{
		FGameplayTagContainer TagContainer(InputTag);
		ASC->CancelAbilities(&TagContainer);
	}
}


void APDPlayerController::UpdateAimRotation()
{
	APawn* ControlledPawn =GetPawn();
	if (!ControlledPawn) return;

	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, true, Hit)) return;

	FVector AimDirection=Hit.Location-ControlledPawn->GetActorLocation();
	AimDirection.Z=0.f;

	if (!AimDirection.IsNearlyZero())
	{
		ControlledPawn->SetActorRotation(AimDirection.Rotation());
	}
}
