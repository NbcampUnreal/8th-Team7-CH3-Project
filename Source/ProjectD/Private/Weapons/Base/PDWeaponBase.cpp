#include "Weapons/Base/PDWeaponBase.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/PDPlayerCharacter.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Component/PDWeaponComponent.h"

#include "Items/PDInventoryComponent.h"

APDWeaponBase::APDWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
	PickupCollision->SetupAttachment(RootComponent);
	PickupCollision->SetSphereRadius(80.f);

	PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupCollision->SetCollisionObjectType(ECC_WorldDynamic);
	PickupCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	PickupCollision->SetGenerateOverlapEvents(false);
}

void APDWeaponBase::BeginPlay()
{
	Super::BeginPlay();
}

void APDWeaponBase::Interact_Implementation(AActor* Interactor)
{
	if (WeaponOwner.IsValid()) return;

	APDPlayerCharacter* Player = Cast<APDPlayerCharacter>(Interactor);
	if (!Player) return;

	if (ItemID.IsNone())
	{
		OnPickupFailed();
		return;
	}

	UPDInventoryComponent* Inventory = Player->FindComponentByClass<UPDInventoryComponent>();
	if (!Inventory || !Inventory->AddItemByID(ItemID))
	{
		OnPickupFailed();
		return;
	}

	Destroy();
}

void APDWeaponBase::Fire_Implementation() {}
void APDWeaponBase::Reload_Implementation() {}

void APDWeaponBase::OnEquip_Implementation(AActor* NewOwner)
{
	WeaponOwner = NewOwner;
	SetOwner(NewOwner);
	WeaponMesh->SetSimulatePhysics(false);
	PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (EquipSound)
		UGameplayStatics::SpawnSoundAttached(EquipSound, WeaponMesh);
}

void APDWeaponBase::OnUnequip_Implementation()
{
	WeaponOwner = nullptr;
}

void APDWeaponBase::UpgradeLevel()
{
	SetLevel(CurrentLevel + 1);
}

void APDWeaponBase::SetLevel(int32 NewLevel)
{
	CurrentLevel = FMath::Clamp(NewLevel, 1, LevelStats.Num());
	OnWeaponLevelChanged.Broadcast(this, CurrentLevel);
}

void APDWeaponBase::SetDropped(bool bDropped)
{
	bIsDropped = bDropped;

	if (bDropped)
	{
		PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		WeaponMesh->SetSimulatePhysics(true);
	}
	else
	{
		PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetSimulatePhysics(false);
	}
}

const FWeaponLevelStats& APDWeaponBase::GetCurrentStats() const
{
	static FWeaponLevelStats DefaultStats;
	if (LevelStats.IsEmpty()) return DefaultStats;
	return LevelStats[FMath::Clamp(CurrentLevel - 1, 0, LevelStats.Num() - 1)];
}

FVector APDWeaponBase::GetAimDirectionFromOwner(const FVector& StartLocation) const
{
	if (!WeaponOwner.IsValid()) return FVector::ForwardVector;

	if (UPDWeaponComponent* Comp = WeaponOwner->FindComponentByClass<UPDWeaponComponent>())
		return Comp->GetAimDirection(StartLocation);

	return WeaponOwner->GetActorForwardVector();
}
