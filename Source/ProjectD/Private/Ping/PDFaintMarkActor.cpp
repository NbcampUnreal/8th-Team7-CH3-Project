#include "Ping/PDFaintMarkActor.h"
#include "Components/StaticMeshComponent.h"

APDFaintMarkActor::APDFaintMarkActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	SetRootComponent(MeshComp);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APDFaintMarkActor::InitializeFaintMark(int32 InFaintId)
{
	FaintId = InFaintId;
	OnFaintMarkInitialized(FaintId);
}