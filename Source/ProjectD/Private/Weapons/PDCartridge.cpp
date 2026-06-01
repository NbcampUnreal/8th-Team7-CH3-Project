#include "Weapons/PDCartridge.h"

#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "GameplayTag/PDGameplayTags.h"

APDCartridge::APDCartridge()
{
	PrimaryActorTick.bCanEverTick = false;

	CartridgeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CartridgeMesh"));
	SetRootComponent(CartridgeMesh);

	CartridgeMesh->SetCollisionObjectType(ECC_PhysicsBody);
	CartridgeMesh->SetCollisionResponseToAllChannels(ECR_Block);
	CartridgeMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CartridgeMesh->SetCollisionResponseToChannel(ECC_Pawn,   ECR_Ignore);
	CartridgeMesh->SetSimulatePhysics(true);
	CartridgeMesh->SetEnableGravity(true);
	CartridgeMesh->SetNotifyRigidBodyCollision(true);
}

void APDCartridge::BeginPlay()
{
	Super::BeginPlay();

	CartridgeMesh->OnComponentHit.AddDynamic(this, &APDCartridge::OnHit);


	const FVector Impulse = GetActorRightVector() * EjectImpulse
	                      + GetActorUpVector()    * (EjectImpulse * 0.5f);
	CartridgeMesh->AddImpulse(Impulse, NAME_None, true);


	GetWorldTimerManager().SetTimer(
		DestroyTimerHandle,
		FTimerDelegate::CreateLambda([this]()
		{
			if (IsValid(this)) Destroy();
		}),
		DestroyDelay, false);
}

void APDCartridge::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (FallSound)
	{
		FGameplayCueParameters Params;
		Params.Location = GetActorLocation();
		Params.Normal = Hit.ImpactNormal;
		Params.SourceObject = FallSound;
		UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(
			this,
			PDGameplayTags::GameplayCue_Weapon_CartridgeHit,
			EGameplayCueEvent::Executed,
			Params);
	}

	Destroy();
}
