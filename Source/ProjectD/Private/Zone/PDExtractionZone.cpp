#include "Zone/PDExtractionZone.h"
#include "Components/BoxComponent.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"

APDExtractionZone::APDExtractionZone()
{
	PrimaryActorTick.bCanEverTick = false;

	OverlapVolume=CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapVolume"));
	OverlapVolume->SetCollisionProfileName(TEXT("Trigger"));
	RootComponent=OverlapVolume;
}

void APDExtractionZone::BeginPlay()
{
	Super::BeginPlay();

	OverlapVolume->OnComponentBeginOverlap.AddDynamic(this, &APDExtractionZone::OnOverlapBegin);
	OverlapVolume->OnComponentEndOverlap.AddDynamic(this, &APDExtractionZone::OnOverlapEnd);
}

void APDExtractionZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn=Cast<APawn>(OtherActor);
	if (!Pawn) return;

	APDPlayerController* PC=Cast<APDPlayerController>(Pawn->GetController());
	if (!PC) return;

	if (ExtractionDelay <= 0.f)
	{
		TriggerExtraction(PC);
		return;
	}
}

void APDExtractionZone::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* Pawn=Cast<APawn>(OtherActor);
	if (!Pawn) return;

	//≈ª√‚?
}

void APDExtractionZone::TriggerExtraction(APlayerController* PC)
{
	if (APDPlayerController* PDPC=Cast<APDPlayerController>(PC))
	{
		PDPC->RequestExtraction();
	}
}