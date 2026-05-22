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

	// 결산 위젯 페이드 종료 시 클라이언트가 PC->Server_RequestBaseTravel 경유로 호출.
	// 모든 PC가 ACK하거나 TravelGateTimeoutSeconds 경과 시 ServerTravel 발동.
	void NotifyPlayerReadyForTravel(APlayerController* PC);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Raid")
	ERaidState CurrentRaidState=ERaidState::Idle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Raid", meta=(ClampMin="0.0"))
	float DeathToTravelDelay = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Raid", meta=(ClampMin="1.0"))
	float TravelGateTimeoutSeconds = 30.f;

	void SetRaidState(ERaidState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category="PD|Raid")
	void OnRaidStateChanged(ERaidState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category="PD|Raid")
	void OnRaidEnded(bool bSuccess);

	void RequestBaseTravel();

private:
	void InitializePlayerInventoryFromLoadout(APlayerController* PC);
	void TransferPlayerInventoryToStash(APlayerController* PC);

	bool AreAllPlayersReadyForTravel() const;
	void OnTravelTimeoutExpired();

	TArray<TWeakObjectPtr<APlayerController>> ReadyPlayersForTravel;
	FTimerHandle TravelTimeoutTimerHandle;
	bool bTravelStarted = false;
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