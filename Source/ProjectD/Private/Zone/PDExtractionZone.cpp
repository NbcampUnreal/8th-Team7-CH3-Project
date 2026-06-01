#include "Zone/PDExtractionZone.h"
#include "Components/BoxComponent.h"
#include "Core/PDGameMode.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

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
	UE_LOG(LogTemp, Warning, TEXT("[Diag-Zone] ExtractionZone OverlapBegin: Other=%s HasAuth=%d"),
		*GetNameSafe(OtherActor), HasAuthority() ? 1 : 0);

	if (!HasAuthority()) return;

	APawn* Pawn=Cast<APawn>(OtherActor);
	if (!Pawn) { UE_LOG(LogTemp, Warning, TEXT("[Diag-Zone] ExtractionZone: Other is not a Pawn")); return; }

	APlayerController* PC=Cast<APlayerController>(Pawn->GetController());
	if (!PC) { UE_LOG(LogTemp, Warning, TEXT("[Diag-Zone] ExtractionZone: Pawn has no PlayerController")); return; }

	if (APDGameMode* GameMode = GetWorld()->GetAuthGameMode<APDGameMode>())
	{
		GameMode->NotifyZoneOccupancy(EPDZoneTravelType::Extraction, PC, true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Diag-Zone] ExtractionZone: no APDGameMode (GetAuthGameMode failed)"));
	}
}

void APDExtractionZone::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority()) return;

	APawn* Pawn=Cast<APawn>(OtherActor);
	if (!Pawn) return;

	APlayerController* PC=Cast<APlayerController>(Pawn->GetController());
	if (!PC) return;

	UE_LOG(LogTemp, Warning, TEXT("[Diag-Zone] ExtractionZone OverlapEnd: PC=%s"), *GetNameSafe(PC));

	if (APDGameMode* GameMode = GetWorld()->GetAuthGameMode<APDGameMode>())
	{
		GameMode->NotifyZoneOccupancy(EPDZoneTravelType::Extraction, PC, false);
	}
}