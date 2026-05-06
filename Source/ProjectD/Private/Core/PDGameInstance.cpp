#include "Core/PDGameInstance.h"
#include "Core/PDSaveGame.h"
#include "Kismet/GameplayStatics.h"

void UPDGameInstance::Init()
{
	Super::Init();
	LoadFromDisk();
}

void UPDGameInstance::SetPlayerData(const FPDPlayerData& InData)
{
	PlayerData=InData;
}

void UPDGameInstance::SaveToDisk()
{
	UPDSaveGame* SaveObject=Cast<UPDSaveGame>(UGameplayStatics::CreateSaveGameObject(UPDSaveGame::StaticClass()));
	if (!SaveObject) return;
	SaveObject->PlayerData=PlayerData;
	UGameplayStatics::SaveGameToSlot(SaveObject, UPDSaveGame::SlotName, UPDSaveGame::UserIndex);
}

void UPDGameInstance::LoadFromDisk()
{
	if (!UGameplayStatics::DoesSaveGameExist(UPDSaveGame::SlotName, UPDSaveGame::UserIndex)) return;
	UPDSaveGame* SaveObject=Cast<UPDSaveGame>(UGameplayStatics::LoadGameFromSlot(UPDSaveGame::SlotName, UPDSaveGame::UserIndex));
	if (!SaveObject) return;
	PlayerData=SaveObject->PlayerData;
}