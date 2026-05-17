#include "Weapons/PDCartridge.h"
#include "Kismet/GameplayStatics.h"

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

	// EjectImpulse: 배출구 기준 측면·상방 방향으로 튀어나가게
	const FVector Impulse = GetActorRightVector() * EjectImpulse
	                      + GetActorUpVector()    * (EjectImpulse * 0.5f);
	CartridgeMesh->AddImpulse(Impulse, NAME_None, true);

	// 안전망: 바닥에 닿지 않아도 일정 시간 후 제거
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
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FallSound, GetActorLocation());

	Destroy();
}
