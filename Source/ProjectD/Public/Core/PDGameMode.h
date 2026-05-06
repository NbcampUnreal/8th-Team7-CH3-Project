#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Type/Types.h"
#include "PDGameMode.generated.h"

class APDPlayerController;

UCLASS(abstract)
class PROJECTD_API APDGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APDGameMode();
	
	UFUNCTION(BlueprintCallable, Category="PD|Raid")
	void StartRaid();
	
	UFUNCTION(BlueprintCallable, Category="PD|Raid")
	void RequestExtraction(APlayerController* PC);
	
	UFUNCTION(BlueprintCallable, Category="PD|Raid")
	void EndRaid(bool bSuccess);
	
	void OnPlayerDied(APlayerController* PC, AActor* Killer);
	FORCEINLINE ERaidState GetRaidState() { return CurrentRaidState; }
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Raid")
	ERaidState CurrentRaidState=ERaidState::Idle;
	
	void SetRaidState(ERaidState NewState);
	
	UFUNCTION(BlueprintImplementableEvent, Category="PD|Raid")
	void OnRaidStateChanged(ERaidState NewState);
	
	UFUNCTION(BlueprintImplementableEvent, Category="PD|Raid")
	void OnRaidEnded(bool bSuccess);
	
	
};
/*
탈출하게 될 경우 
RequestExtraction()
→ EndRaid(true)
→ SaveToDisk()    
→ OnRaidEnded(true)  

탈출하지 못하고 죽을 경우       
EndRaid(false)
-> OnRaidEnded(false) BP에서 사망 UI -> 레벨 전환
 */