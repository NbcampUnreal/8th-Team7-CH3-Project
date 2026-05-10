#include "Ping/PDPingMarker.h"
#include "Components/StaticMeshComponent.h"

APDPingMarker::APDPingMarker()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); //충돌 x
	SetRootComponent(MeshComp);
}

void APDPingMarker::InitializePing(EPDPingType InPingType)
{
	OnPingInitialized(InPingType);
}