#include "Cover/PDCoverBase.h"
#include "Components/StaticMeshComponent.h"
#include "GeometryCollection/GeometryCollectionComponent.h"

APDCoverBase::APDCoverBase()
{
	CoverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoverMesh"));
	SetRootComponent(CoverMesh);
	CoverMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	DestructionCollection = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("DestructionCollection"));
	DestructionCollection->SetupAttachment(RootComponent);
	DestructionCollection->SetHiddenInGame(true);
	DestructionCollection->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APDCoverBase::BeginPlay()
{
	Super::BeginPlay();
	CurrentHP = MaxHP;
}

FVector APDCoverBase::GetSnapLocation(AActor* Requester) const
{
	if (!CoverMesh || !Requester) return GetActorLocation();

	const FVector CoverCenter = CoverMesh->GetComponentLocation();

	FVector ToCharacter = Requester->GetActorLocation() - CoverCenter;
	ToCharacter.Z = 0.f;
	ToCharacter = ToCharacter.GetSafeNormal();

	const FBoxSphereBounds Bounds = CoverMesh->CalcBounds(CoverMesh->GetComponentTransform());
	const FVector Extent = Bounds.BoxExtent;
	const float HalfExtent = FMath::Abs(ToCharacter.X) * Extent.X
		+ FMath::Abs(ToCharacter.Y) * Extent.Y;

	FVector SnapPos = CoverCenter + ToCharacter * (HalfExtent + CharacterClearance);
	SnapPos.Z = Requester->GetActorLocation().Z;

	return SnapPos;
}

FRotator APDCoverBase::GetSnapRotation(AActor* Requester) const
{
	if (!Requester) return GetActorRotation();
	FVector TowardWall = GetActorLocation() - Requester->GetActorLocation();
	TowardWall.Z = 0.f;
	return TowardWall.GetSafeNormal().Rotation();
}

bool APDCoverBase::TryOccupy(AActor* Requester)
{
	if (Occupant.IsValid()) return false;
	Occupant = Requester;
	return true;
}

void APDCoverBase::Release(AActor* Requester)
{
	if (Occupant == Requester)
	{
		Occupant = nullptr;
	}
}

void APDCoverBase::TakeCoverDamage(float Damage)
{
	if (CoverState == ECoverState::Destroyed) return;
	CurrentHP = FMath::Max(CurrentHP - Damage, 0.f);

	BP_OnCoverDamaged(Damage, CurrentHP, MaxHP);

	if (CurrentHP <= 0.f)
		SetCoverState(ECoverState::Destroyed);
	else if (CurrentHP <= MaxHP * DamagedThreshold)
		SetCoverState(ECoverState::Damaged);
}

void APDCoverBase::SetCoverState(ECoverState NewState)
{
	if (CoverState == NewState) return;
	CoverState = NewState;

	BP_OnCoverStateChanged(CoverState);

	if (CoverState == ECoverState::Destroyed)
		OnDestroyed_Internal();
}

void APDCoverBase::OnDestroyed_Internal()
{
	CoverMesh->SetHiddenInGame(true);
	CoverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (IsValid(DestructionCollection))
	{
		DestructionCollection->SetHiddenInGame(false);
		DestructionCollection->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		DestructionCollection->SetSimulatePhysics(true);
	}

	BP_OnCoverDestroyed();
	OnCoverDestroyed.ExecuteIfBound();
	Occupant = nullptr;
}
