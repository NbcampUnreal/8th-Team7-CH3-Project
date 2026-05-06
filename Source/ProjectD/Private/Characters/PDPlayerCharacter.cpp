#include "Characters/PDPlayerCharacter.h"

#include "AttributeSet/PDAttributeSet.h"
#include "Camera/CameraComponent.h"
#include "Component/PDVisionComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

APDPlayerCharacter::APDPlayerCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	CameraBoom=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;

	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	VisionComponent=CreateDefaultSubobject<UPDVisionComponent>(TEXT("VisionComponent"));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void APDPlayerCharacter::InitAbilitySystem()
{
	Super::InitAbilitySystem();
	
	auto ApplyInfiniteGE=[&](TSubclassOf<UGameplayEffect> GEClass)
	{
		if (!GEClass) return;
		FGameplayEffectContextHandle Context=ASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec=ASC->MakeOutgoingSpec(GEClass, 1.f, Context);
		if (Spec.IsValid())
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	};
	
	if (UPDVisionComponent* Vision=FindComponentByClass<UPDVisionComponent>())
	{
		Vision->BindToAttributeSet(ASC);
	}
	
	ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetStaminaAttribute())
	.AddUObject(this, &APDPlayerCharacter::OnStaminaChanged);

	ApplyInfiniteGE(HungerDecayEffectClass);
	ApplyInfiniteGE(ThirstDecayEffectClass);
}

void APDPlayerCharacter::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (!AttributeSet||!VisionComponent) return;
	const float MaxStamina=AttributeSet->GetMaxStamina();
	if (MaxStamina<=0.f) return;
	VisionComponent->UpdateStaminaScale(Data.NewValue/MaxStamina);
}