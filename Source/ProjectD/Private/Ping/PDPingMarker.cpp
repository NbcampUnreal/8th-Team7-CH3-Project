#include "Ping/PDPingMarker.h"
#include "Components/StaticMeshComponent.h"

APDPingMarker::APDPingMarker()
{
	PrimaryActorTick.bCanEverTick = false;
	
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); //충돌 x
	MeshComp->SetupAttachment(SceneRoot);
}

void APDPingMarker::InitializePing(EPDPingType InPingType)
{
	OnPingInitialized(InPingType);
}