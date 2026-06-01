#include "Zone/PDRaidEntryPortal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Core/PDGameInstance.h"
#include "Core/PDGameMode.h"
#include "Core/PDPlayerController.h"
#include "Core/PDPlayerState.h"

APDRaidEntryPortal::APDRaidEntryPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	OverlapVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapVolume"));
	OverlapVolume->SetCollisionProfileName(TEXT("Trigger"));
	RootComponent = OverlapVolume;
}

void APDRaidEntryPortal::BeginPlay()
{
	Super::BeginPlay();
	OverlapVolume->OnComponentBeginOverlap.AddDynamic(this, &APDRaidEntryPortal::OnOverlapBegin);
	OverlapVolume->OnComponentEndOverlap.AddDynamic(this, &APDRaidEntryPortal::OnOverlapEnd);
}

void APDRaidEntryPortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("[Diag-Zone] RaidEntryPortal OverlapBegin: Other=%s HasAuth=%d"),
		*GetNameSafe(OtherActor), HasAuthority() ? 1 : 0);

	if (!HasAuthority())
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) { UE_LOG(LogTemp, Warning, TEXT("[Diag-Zone] Portal: Other is not a Pawn")); return; }

	APDPlayerController* PC = Cast<APDPlayerController>(Pawn->GetController());
	if (!PC) { UE_LOG(LogTemp, Warning, TEXT("[Diag-Zone] Portal: Pawn has no APDPlayerController")); return; }

	// 진입 사전 준비(로드아웃 확정). 트래블은 카운트다운 만료 시 GameMode 가 수행.
	PrepareEntry(PC);

	if (APDGameMode* GameMode = GetWorld()->GetAuthGameMode<APDGameMode>())
	{
		GameMode->NotifyZoneOccupancy(EPDZoneTravelType::RaidEntry, PC, true, RaidLevel);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Diag-Zone] Portal: no APDGameMode (GetAuthGameMode failed)"));
	}
}

void APDRaidEntryPortal::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) return;

	APDPlayerController* PC = Cast<APDPlayerController>(Pawn->GetController());
	if (!PC) return;

	if (APDGameMode* GameMode = GetWorld()->GetAuthGameMode<APDGameMode>())
	{
		GameMode->NotifyZoneOccupancy(EPDZoneTravelType::RaidEntry, PC, false);
	}
}

void APDRaidEntryPortal::PrepareEntry(APlayerController* PC)
{
	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!GI) return;

	APDPlayerState* PDPlayerState = PC ? PC->GetPlayerState<APDPlayerState>() : nullptr;
	if (!PDPlayerState) return;

	const FString SaveKey = GI->GetSaveKeyForController(PC);
	const FPDPlayerData Data = GI->LoadPlayerDataFromDisk(SaveKey, PC->IsLocalController());
	PDPlayerState->InitializePersistentData(Data);
	PDPlayerState->ConfirmRaidLoadout(Data.StashItems, Data.Gold);

	GI->SavePlayerDataToDisk(SaveKey, PDPlayerState->GetPersistentData());

	OnRaidEntryTriggered(PC);
}
