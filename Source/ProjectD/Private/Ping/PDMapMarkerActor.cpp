#include "Ping/PDMapMarkerActor.h"
#include "Components/StaticMeshComponent.h"

APDMapMarkerActor::APDMapMarkerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	SetRootComponent(MeshComp);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APDMapMarkerActor::InitializeMarker(int32 InMarkerId, int32 InDisplayIndex)
{
	MarkerId = InMarkerId;
	DisplayIndex = InDisplayIndex;
	OnMarkerInitialized(MarkerId, DisplayIndex);
}

void APDMapMarkerActor::UpdateDisplayIndex(int32 InDisplayIndex)
{
	if (DisplayIndex == InDisplayIndex) return;
	DisplayIndex = InDisplayIndex;
	OnDisplayIndexUpdated(DisplayIndex);
}