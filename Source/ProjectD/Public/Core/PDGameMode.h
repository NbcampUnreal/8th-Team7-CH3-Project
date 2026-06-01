#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "Type/Types.h"
#include "PDGameMode.generated.h"

class APDPlayerController;
struct FPDPlayerRaidEntryData;

UCLASS(abstract)
class PROJECTD_API APDGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APDGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;

	// seamless travel 시 PostLogin 미발화 → 트래블 플레이어에 트랜지션 push + AutoStart 디바운스 동일 처리.
	virtual void HandleSeamlessTravelPlayer(AController*& C) override;

	UFUNCTION(BlueprintCallable, Category="PD|Raid")
	void StartRaid();

	UFUNCTION(BlueprintCallable, Category="PD|Raid|Travel")
	void TravelToRaidLevel(FName RaidMapName);

	UFUNCTION(BlueprintCallable, Category="PD|Raid|Travel")
	void TravelToBaseLevel(bool bMarkResetPending);

	UFUNCTION(BlueprintCallable, Category="PD|Raid")
	void RequestExtraction(APlayerController* PC);

	UFUNCTION(BlueprintCallable, Category="PD|Raid")
	void EndRaid(bool bSuccess);

	// ─── 디버그 전용: 호스트가 즉시 라이드 종료 강제 (전원 일괄 처리 + EndRaid + 트래블) ───
	// 성공: 전원 추출 처리 → 인벤토리 stash 이체(유지). 실패: 전원 사망 처리 → 인벤토리 드롭(소실).
	UFUNCTION(BlueprintCallable, Category="PD|Raid|Debug")
	void DebugForceRaidSuccess();

	UFUNCTION(BlueprintCallable, Category="PD|Raid|Debug")
	void DebugForceRaidFail();

	void OnPlayerDied(APlayerController* PC, AActor* Killer);
	FORCEINLINE ERaidState GetRaidState() { return CurrentRaidState; }

	// 결산 위젯 페이드 종료 시 클라이언트가 PC->Server_RequestBaseTravel 경유로 호출.
	// 모든 PC가 ACK하거나 TravelGateTimeoutSeconds 경과 시 ServerTravel 발동.
	void NotifyPlayerReadyForTravel(APlayerController* PC);

	// 존(Portal/Extraction) 액터가 overlap begin/end 시 호출(서버 전용).
	// 점유 인원 절반 이상 → 카운트다운 시작, 전원 → 단축, 만료 시 트래블/추출.
	// RaidEntry 는 진입할 레이드 레벨 에셋을 함께 전달.
	void NotifyZoneOccupancy(EPDZoneTravelType ZoneType, APlayerController* PC, bool bEntered, TSoftObjectPtr<UWorld> RaidLevel = nullptr);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Raid")
	ERaidState CurrentRaidState=ERaidState::Idle;

	// 결산 위젯이 떠 있는 동안 전체 PC가 좌클릭(ACK)하지 않더라도 강제 트래블되는 시간.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Raid", meta=(ClampMin="1.0", ForceUnits="s"))
	float TravelGateTimeoutSeconds = 20.f;

	// 이 목록에 포함된 맵에서만 PostLogin 디바운스로 StartRaid 자동 호출.
	// 단일 GameMode 클래스를 여러 레벨이 공유하므로 라이드 전용 맵만 골라낼 수단.
	// 라이드 맵(Debug 포함) 을 콘텐츠 브라우저에서 픽해서 추가.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Raid|Autostart")
	TArray<TSoftObjectPtr<UWorld>> AutoStartRaidLevels;

	// 마지막 PostLogin 후 새 합류가 없을 때까지 기다리는 시간. 만료 시 StartRaid 1회 발사.
	// PIE Listen 에서 클라가 합류할 시간을 주기 위함. 호스트 PostLogin 후 클라 합류까지 보통 1~2초.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Raid|Autostart", meta=(ClampMin="0.1", ForceUnits="s"))
	float AutoStartDebounceSeconds = 3.0f;

	// 라이드 진입 연출 위젯에 표시할 구역/맵 이름. 비워두면 위젯이 텍스트 숨김.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Raid")
	FText RaidZoneDisplayName;

	void SetRaidState(ERaidState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category="PD|Raid")
	void OnRaidStateChanged(ERaidState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category="PD|Raid")
	void OnRaidEnded(bool bSuccess);

	void RequestBaseTravel();

private:
	// Step 2-B: 사망 시 죽은 PC를 첫 생존자로 관전 전환, 기존 관전자들 중 죽은 PC를
	// 보고 있던 사람들은 다음 생존자로 순환. AffectedPC 는 방금 죽거나 추출된 PC.
	void HandleSpectatorTransitionForDeath(APlayerController* DeadPC);
	void NotifyOthersWatchingFinalized(APlayerController* AffectedPC);
	APlayerController* FindFirstAliveSpectateTarget(APlayerController* ExcludePC) const;

	// 마지막 Alive 캐릭터 사망 직후 호출. 남은 Downed 동료 전부 강제 HandleDeath.
	// 배그식 흐름: 마지막 생존자 죽으면 BleedOut 기다리지 않고 즉시 결산으로.
	void FinalizeStrandedDownedPlayers();

	// Step 2-C: 결산 스탯 추적.
	void CaptureInitialRaidSnapshotFor(APlayerController* PC);
	void FinalizeRaidStatsOnDeath(APlayerController* DeadPC);
	void FinalizeRaidStatsOnExtraction(APlayerController* ExtractedPC);
	void CreditKillToShooter(AActor* Killer, APlayerController* Victim);
	float GetCurrentRaidElapsedSeconds() const;
	int32 CountInventoryItemQuantity(class UPDInventoryComponent* Inventory) const;

	// 옵션 B: 결산 위젯 entries 서버 빌드 (GameState->PlayerArray + PDPlayerState).
	void BuildRaidEndEntries(TArray<FPDPlayerRaidEntryData>& OutEntries) const;

	// 서버 권한 GameMode 만 사용. RaidStats.SurvivalSeconds 계산 기준점.
	float RaidStartServerTime = 0.f;

	void InitializePlayerStateFromSave(APlayerController* PC);
	void SavePlayerStateToDisk(APlayerController* PC);
	void InitializePlayerInventoryFromLoadout(APlayerController* PC);
	void TransferPlayerInventoryToStash(APlayerController* PC);

	// 모든 참가자가 Dead 또는 Extracted 상태가 되었는지 검사하고, 그 경우 EndRaid 발동.
	void EvaluateRaidEnd();
	// 한 PC 의 추출 후처리(인벤토리 이체, 상태 저장, SecureContainer 보존). 멀티 자체는 그대로 진행.
	void ProcessExtractionForPlayer(APlayerController* PC);
	// 한 PC 의 사망 후처리(인벤토리 손실 = 이체 안 함, 상태만 저장).
	void ProcessDeathForPlayer(APlayerController* PC);

	bool AreAllPlayersReadyForTravel() const;
	void OnTravelTimeoutExpired();

	int32 CountAlivePlayers() const;
	int32 CountExtractedPlayers() const;
	int32 CountDeadPlayers() const;

	TArray<TWeakObjectPtr<APlayerController>> ReadyPlayersForTravel;
	FTimerHandle TravelTimeoutTimerHandle;
	bool bTravelStarted = false;

	// StartRaid 재진입 방지. ServerTravel 마다 새 GameMode 인스턴스라 자연스럽게 false 로 시작.
	bool bRaidStarted = false;
	FTimerHandle AutoStartDebounceHandle;
	void HandleAutoStartDebounceFired();

	// PostLogin / HandleSeamlessTravelPlayer 공용 — 디바운스 무장 + 트랜지션 push.
	void ArmAutoStartDebounce(APlayerController* ContextPC);
	void PushRaidStartTransitionTo(APlayerController* PC);

	// 현재 맵이 AutoStartRaidLevels 화이트리스트에 있는지. PIE prefix 제거 후 비교.
	bool IsCurrentMapAutoStartEnabled() const;

	// ─── 존 점유 카운트다운 ──────────────────────────────────────────────
	// 점유 변경 시 재평가: 절반 이상이면 시작/유지, 전원이면 3초로 단축, 미만이면 취소.
	void EvaluateZoneCountdown();
	// 타이머 만료 → 트래블(RaidEntry) 또는 존 안 인원 추출(Extraction).
	void OnZoneCountdownFired();
	class APDGameState* GetPDGameState() const;

	TSet<TWeakObjectPtr<APlayerController>> ZoneOccupants;
	EPDZoneTravelType CurrentZoneType = EPDZoneTravelType::None;
	TSoftObjectPtr<UWorld> PendingRaidLevel;
	float ZoneEndServerTime = -1.f;
	bool bZoneFinalCountdownActive = false;
	FTimerHandle ZoneCountdownTimerHandle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Zone", meta=(ClampMin="1.0", ForceUnits="s", AllowPrivateAccess="true"))
	float ZoneCountdownSeconds = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|Zone", meta=(ClampMin="0.5", ForceUnits="s", AllowPrivateAccess="true"))
	float ZoneFinalCountdownSeconds = 3.f;
};
