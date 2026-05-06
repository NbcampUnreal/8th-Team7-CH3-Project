#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Type/Types.h"
#include "PDSaveGame.generated.h"

UCLASS()
class PROJECTD_API UPDSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	FPDPlayerData PlayerData;

	static const FString SlotName;
	static const int32 UserIndex;
};
