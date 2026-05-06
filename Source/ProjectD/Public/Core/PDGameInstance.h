#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Type/Types.h"
#include "PDGameInstance.generated.h"

UCLASS()
class PROJECTD_API UPDGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	
	UFUNCTION(BlueprintCallable, Category = "PD|Save")
	void SetPlayerData(const FPDPlayerData& InData);
	
	UFUNCTION(BlueprintCallable, Category = "PD|Save")
	FPDPlayerData GetPlayerData() const { return PlayerData; }
	
	UFUNCTION(BlueprintCallable, Category = "PD|Save")
	void SaveToDisk();
	
	UFUNCTION(BlueprintCallable, Category = "PD|Save")
	void LoadFromDisk();

protected:
	UPROPERTY()
	FPDPlayerData PlayerData;
};
