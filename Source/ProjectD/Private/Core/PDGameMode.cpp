#include "Core/PDGameMode.h"
#include "Core/PDGameState.h"
#include "Core/PDGameInstance.h"
#include "Core/PDPlayerController.h"
#include "Subsystems/PDLoadingScreenSubsystem.h"
#include "Items/Containers/PDSecureContainerComponent.h"
#include "Core/PDPlayerState.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Game/PDRaidStats.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Type/Types.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDRaid, Log, All);

APDGameMode::APDGameMode()
{
	PlayerStateClass = APDPlayerState::StaticClass();
	GameStateClass = APDGameState::StaticClass();
	bUseSeamlessTravel = true;
}

void APDGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	InitializePlayerStateFromSave(NewPlayer);

	UE_LOG(LogPDRaid, Log, TEXT("PostLogin: PC=%s NumPlayers=%d"),
		*GetNameSafe(NewPlayer),
		GetWorld() ? GetWorld()->GetNumPlayerControllers() : -1);

	// 라이드 진입 연출은 도착 즉시 (디바운스 대기 X) — 로딩스크린과 매끄럽게 이어지도록.
	PushRaidStartTransitionTo(NewPlayer);

	// StartRaid 로직은 디바운스 (전원 합류 대기).
	ArmAutoStartDebounce(NewPlayer);
}

void APDGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	Super::HandleSeamlessTravelPlayer(C);

	// seamless travel 시 PostLogin 이 안 불리므로, 트래블한 플레이어에 대해 동일 처리.
	APlayerController* PC = Cast<APlayerController>(C);
	if (!PC) return;

	// PostLogin 과 동일하게 디스크에서 영구 데이터(stash/loadout 등) 재로드.
	// 이게 없으면 seamless travel 시 추출한 stash/로드아웃이 트래블 너머로 유실됨.
	InitializePlayerStateFromSave(PC);

	UE_LOG(LogPDRaid, Log, TEXT("HandleSeamlessTravelPlayer: PC=%s NumPlayers=%d"),
		*GetNameSafe(PC),
		GetWorld() ? GetWorld()->GetNumPlayerControllers() : -1);

	PushRaidStartTransitionTo(PC);
	ArmAutoStartDebounce(PC);
}

void APDGameMode::ArmAutoStartDebounce(APlayerController* ContextPC)
{
	// 매 합류마다 디바운스 리셋. 마지막 합류 후 AutoStartDebounceSeconds 동안 새 합류 없으면 StartRaid 1회.
	// 화이트리스트 미포함 맵 (Base/Startup 등) 에서는 발사 안 됨.
	if (bRaidStarted || !IsCurrentMapAutoStartEnabled()) return;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoStartDebounceHandle);
		World->GetTimerManager().SetTimer(
			AutoStartDebounceHandle,
			this,
			&APDGameMode::HandleAutoStartDebounceFired,
			AutoStartDebounceSeconds,
			false);
		UE_LOG(LogPDRaid, Verbose, TEXT("AutoStart: debounce armed (%.2fs) after PC=%s"),
			AutoStartDebounceSeconds, *GetNameSafe(ContextPC));
	}
}

void APDGameMode::PushRaidStartTransitionTo(APlayerController* PC)
{
	if (!IsCurrentMapAutoStartEnabled()) return;
	if (APDPlayerController* PDPC = Cast<APDPlayerController>(PC))
	{
		PDPC->Client_ShowRaidStartTransition(RaidZoneDisplayName);
	}
}

void APDGameMode::HandleAutoStartDebounceFired()
{
	if (bRaidStarted) return;
	UE_LOG(LogPDRaid, Log, TEXT("AutoStart: debounce fired, calling StartRaid (NumPlayers=%d)"),
		GetWorld() ? GetWorld()->GetNumPlayerControllers() : -1);
	StartRaid();
}

bool APDGameMode::IsCurrentMapAutoStartEnabled() const
{
	UWorld* World = GetWorld();
	if (!World) return false;

	// PIE prefix (UEDPIE_N_) ?�거??long package name ?�로 비교.
	const FString CurrentMapName = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());

	for (const TSoftObjectPtr<UWorld>& MapRef : AutoStartRaidLevels)
	{
		if (MapRef.IsNull()) continue;
		if (CurrentMapName == MapRef.ToSoftObjectPath().GetLongPackageName())
		{
			return true;
		}
	}
	return false;
}

void APDGameMode::StartRaid()
{
	if (bRaidStarted)
	{
		UE_LOG(LogPDRaid, Verbose, TEXT("StartRaid: already started, ignored"));
		return;
	}
	bRaidStarted = true;

	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	RaidStartServerTime = GS ? GS->GetServerWorldTimeSeconds() : (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);

	int32 PlayerCount = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		InitializePlayerInventoryFromLoadout(PC);

		if (APDPlayerState* PS = PC ? PC->GetPlayerState<APDPlayerState>() : nullptr)
		{
			PS->ResetRaidParticipationState();

			// 로드아웃 리셋(ResetInventory)이 인벤토리 기반 퀵슬롯 보관 아이템을 지우므로 여기서 재복원.
			// 장비는 인벤토리 리셋에 영향 없지만 idempotent 하게 한 번 더 호출(이미 장착 시 skip).
			PS->RestoreEquippedItemsFromPersistent();
			PS->RestoreQuickSlotItemsFromPersistent();
		}

		// Step 2-C: 초기 골드/?�이???�량 ?�냅??(?�망/추출 ?�점??Delta 계산??.
		CaptureInitialRaidSnapshotFor(PC);
		++PlayerCount;
	}

	SetRaidState(ERaidState::InProgress);
	UE_LOG(LogPDRaid, Log, TEXT("StartRaid: Participants=%d StartTime=%.2f"), PlayerCount, RaidStartServerTime);

	// 진입 연출(Client_ShowRaidStartTransition)은 PostLogin/HandleSeamlessTravelPlayer 에서
	// 도착 즉시 push 됨 — StartRaid 디바운스와 분리 (로딩스크린 직후 검정 커버).
}

void APDGameMode::TravelToRaidLevel(FName RaidMapName)
{
	if (RaidMapName.IsNone())
	{
		UE_LOG(LogPDRaid, Warning, TEXT("TravelToRaidLevel: RaidMapName is not set."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	// 호스트 측 LoadingScreen 즉시 표시 — 클라는 PreClientTravel 에서 자동 ShowImmediate.
	// 이게 없으면 호스트는 PreLoadMap(실제 로드) 때야 떠서 클라보다 늦게 보임.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPDLoadingScreenSubsystem* LSS = GI->GetSubsystem<UPDLoadingScreenSubsystem>())
		{
			LSS->ShowImmediate();
		}
	}

	UE_LOG(LogPDRaid, Log, TEXT("TravelToRaidLevel: ServerTravel to %s"), *RaidMapName.ToString());

	// 1프레임 지연 후 ServerTravel — LoadingScreen 이 먼저 렌더된 뒤 cleanup 시작되도록
	// (즉시 호출하면 같은 프레임에 cleanup 점유로 LoadingScreen 이 한 프레임도 안 보임).
	// StartRaid 는 트래블 후 라이드 맵 GameMode 가 PostLogin/Seamless 디바운스로 자동 호출.
	World->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this, RaidMapName]()
		{
			if (UWorld* W = GetWorld())
			{
				W->ServerTravel(RaidMapName.ToString());
			}
		}));
}

void APDGameMode::TravelToBaseLevel(bool bMarkResetPending)
{
	if (UPDGameInstance* GI = GetGameInstance<UPDGameInstance>())
	{
		UE_LOG(LogPDRaid, Log, TEXT("TravelToBaseLevel: bMarkResetPending=%d"), bMarkResetPending ? 1 : 0);
		GI->TravelToLevel(GI->GetBaseLevel(), bMarkResetPending);
	}
}

void APDGameMode::RequestExtraction(APlayerController* PC)
{
	if (!PC)
	{
		UE_LOG(LogPDRaid, Warning, TEXT("RequestExtraction: null PC, ignored"));
		return;
	}

	if (CurrentRaidState != ERaidState::InProgress)
	{
		UE_LOG(LogPDRaid, Warning, TEXT("RequestExtraction: ignored, state=%d PC=%s"),
			static_cast<int32>(CurrentRaidState), *PC->GetName());
		return;
	}

	APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
	if (!PS)
	{
		UE_LOG(LogPDRaid, Warning, TEXT("RequestExtraction: PC=%s has no PDPlayerState"), *PC->GetName());
		return;
	}

	if (PS->IsExtracted() || PS->IsRaidDead())
	{
		UE_LOG(LogPDRaid, Warning, TEXT("RequestExtraction: PC=%s already finalized (extracted=%d dead=%d)"),
			*PC->GetName(), PS->IsExtracted() ? 1 : 0, PS->IsRaidDead() ? 1 : 0);
		return;
	}

	UE_LOG(LogPDRaid, Log, TEXT("RequestExtraction: PC=%s -> mark extracted"), *PC->GetName());
	PS->SetExtracted(true);

	// Step 2-C: ?�벤?�리가 stash �???��지�??�에 Delta ?�정.
	FinalizeRaidStatsOnExtraction(PC);

	ProcessExtractionForPlayer(PC);

	// Step 2-B: 추출?��? 관?�하???�른 PC 가 ?�으�??�음 ?�존?�로 ?�환.
	NotifyOthersWatchingFinalized(PC);

	EvaluateRaidEnd();
}

void APDGameMode::OnPlayerDied(APlayerController* PC, AActor* Killer)
{
	if (!PC)
	{
		UE_LOG(LogPDRaid, Warning, TEXT("OnPlayerDied: null PC, ignored"));
		return;
	}
	if (CurrentRaidState == ERaidState::Ended)
	{
		UE_LOG(LogPDRaid, Verbose, TEXT("OnPlayerDied: PC=%s ignored, raid already Ended"), *PC->GetName());
		return;
	}

	// If death arrives while AutoStart is debounced, force StartRaid before evaluating raid end.
	// Otherwise EvaluateRaidEnd can run while the raid is still Idle.
	if (CurrentRaidState == ERaidState::Idle && !bRaidStarted)
	{
		UE_LOG(LogPDRaid, Log, TEXT("OnPlayerDied: state=Idle death race detected; cancelling AutoStart debounce and forcing StartRaid."));
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(AutoStartDebounceHandle);
		}
		StartRaid();
	}

	APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
	if (PS)
	{
		if (PS->IsRaidDead())
		{
			UE_LOG(LogPDRaid, Verbose, TEXT("OnPlayerDied: PC=%s already marked dead"), *PC->GetName());
			return;
		}
		PS->SetRaidDead(true);
	}
	else
	{
		UE_LOG(LogPDRaid, Warning, TEXT("OnPlayerDied: PC=%s GetPlayerState<APDPlayerState> returned NULL"), *PC->GetName());
	}

	UE_LOG(LogPDRaid, Log, TEXT("OnPlayerDied: PC=%s Killer=%s Alive=%d Dead=%d Extracted=%d"),
		*PC->GetName(),
		*GetNameSafe(Killer),
		CountAlivePlayers(), CountDeadPlayers(), CountExtractedPlayers());

	ProcessDeathForPlayer(PC);

	// Step 2-C: ?�망??결산 ?�탯 ?�정 + ?�러?�게 Kill ?�레??
	FinalizeRaidStatsOnDeath(PC);
	CreditKillToShooter(Killer, PC);

	// Step 2-B: ?�망?�는 �??�존???�점?�로 관???�환,
	// 죽�? PC�?보고 ?�던 ?�른 관?�자?��? ?�음 ?�존?�로 ?�동 ?�환.
	HandleSpectatorTransitionForDeath(PC);
	NotifyOthersWatchingFinalized(PC);

	// 배그?? 마�?�?Alive 가 방금 죽었?�면 ?��? Downed ?�료 BleedOut 기다리�? ?�고 즉시 처형.
	FinalizeStrandedDownedPlayers();

	EvaluateRaidEnd();
}

void APDGameMode::EvaluateRaidEnd()
{
	if (CurrentRaidState != ERaidState::InProgress)
	{
		UE_LOG(LogPDRaid, Verbose, TEXT("EvaluateRaidEnd: skipped, state=%d"),
			static_cast<int32>(CurrentRaidState));
		return;
	}

	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	if (!GS || GS->PlayerArray.Num() == 0)
	{
		UE_LOG(LogPDRaid, Warning, TEXT("EvaluateRaidEnd: empty PlayerArray, holding off"));
		return;
	}

	int32 Alive = 0;
	int32 Extracted = 0;
	int32 Dead = 0;
	for (APlayerState* PSBase : GS->PlayerArray)
	{
		APDPlayerState* PS = Cast<APDPlayerState>(PSBase);
		if (!PS) continue;

		if (PS->IsExtracted())     ++Extracted;
		else if (PS->IsRaidDead()) ++Dead;
		else                       ++Alive;
	}

	UE_LOG(LogPDRaid, Log, TEXT("EvaluateRaidEnd: Alive=%d Dead=%d Extracted=%d (Total=%d)"),
		Alive, Dead, Extracted, GS->PlayerArray.Num());

	if (Alive > 0)
	{
		UE_LOG(LogPDRaid, Log, TEXT("EvaluateRaidEnd: %d player(s) still alive, raid continues"), Alive);
		return;
	}

	const bool bAnyExtracted = Extracted > 0;
	UE_LOG(LogPDRaid, Log, TEXT("EvaluateRaidEnd: all participants finalized -> EndRaid(bSuccess=%d)"),
		bAnyExtracted ? 1 : 0);
	EndRaid(bAnyExtracted);
}

void APDGameMode::EndRaid(bool bSuccess)
{
	if (CurrentRaidState == ERaidState::Ended)
	{
		UE_LOG(LogPDRaid, Log, TEXT("EndRaid: already Ended, ignored"));
		return;
	}
	SetRaidState(ERaidState::Ended);

	UE_LOG(LogPDRaid, Log, TEXT("EndRaid: bSuccess=%d Alive=%d Dead=%d Extracted=%d"),
		bSuccess ? 1 : 0,
		CountAlivePlayers(), CountDeadPlayers(), CountExtractedPlayers());

	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();

	// ?�산 마감: 추출?�는 RequestExtraction ?�계?�서 ?��? 처리?? ?�기?�는 ?�망?�만 마�? 처리.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;

		APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
		if (!PS) continue;

		if (PS->IsExtracted())
		{
			UE_LOG(LogPDRaid, Log, TEXT("EndRaid: PC=%s already extracted (kept items/gold)"), *PC->GetName());
			continue;
		}

		// ?�망?? ?�벤?�리/RaidGold ?�실. ?�구 ?�이???�태???�판 ??�??�??
		UE_LOG(LogPDRaid, Log, TEXT("EndRaid: PC=%s dead -> drop raid inventory, save persistent only"),
			*PC->GetName());
		SavePlayerStateToDisk(PC);
	}

	// SecureContainer ?�역 규칙: ?�느 ?�구??추출 �??�으�?메모�?캐시 비�?(?�음 ?�이??빈손).
	if (!bSuccess && GI)
	{
		UE_LOG(LogPDRaid, Log, TEXT("EndRaid: no survivors, clearing GI SecureContainer cache"));
		GI->SetSecureContainerItems({});
	}

	// BP ?�출 ?????�운???�펙?????�버 ?�이???�출?? 결산 ?�젯 ?�시???�래 ClientRPC �??�원??
	OnRaidEnded(bSuccess);

	// ?�션 B: 모든 PC ??결산 ?�젯 push ClientRPC.
	TArray<FPDPlayerRaidEntryData> Entries;
	BuildRaidEndEntries(Entries);
	const float Duration = GetCurrentRaidElapsedSeconds();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APDPlayerController* PDPC = Cast<APDPlayerController>(It->Get());
		if (!PDPC) continue;
		PDPC->Client_ShowRaidEndTransition(bSuccess, Entries, Duration);
	}

	// 모든 PC가 ACK?�거??TravelGateTimeoutSeconds 경과 ??ServerTravel. ??�?빠른 쪽이 발동.
	UE_LOG(LogPDRaid, Log, TEXT("EndRaid: travel gate armed (timeout=%.1fs) Entries=%d Duration=%.1fs"),
		TravelGateTimeoutSeconds, Entries.Num(), Duration);
	GetWorldTimerManager().SetTimer(
		TravelTimeoutTimerHandle,
		this,
		&APDGameMode::OnTravelTimeoutExpired,
		TravelGateTimeoutSeconds,
		false);
}

void APDGameMode::DebugForceRaidSuccess()
{
	if (CurrentRaidState != ERaidState::InProgress)
	{
		UE_LOG(LogPDRaid, Warning, TEXT("DebugForceRaidSuccess: raid not InProgress (state=%d)"),
			static_cast<int32>(CurrentRaidState));
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	// 전원 추출 처리 — 인벤토리를 stash 로 이체(유지). 이후 EndRaid 가 추출자는 드롭 안 함.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;
		APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
		if (!PS || PS->IsExtracted() || PS->IsRaidDead()) continue;

		PS->SetExtracted(true);
		FinalizeRaidStatsOnExtraction(PC);
		ProcessExtractionForPlayer(PC);
	}

	UE_LOG(LogPDRaid, Warning, TEXT("[Debug] ForceRaidSuccess -> EndRaid(true)"));
	EndRaid(true);
}

void APDGameMode::DebugForceRaidFail()
{
	if (CurrentRaidState != ERaidState::InProgress)
	{
		UE_LOG(LogPDRaid, Warning, TEXT("DebugForceRaidFail: raid not InProgress (state=%d)"),
			static_cast<int32>(CurrentRaidState));
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	// 전원 사망 처리(추출 안 한 인원). EndRaid 가 비추출자 인벤토리 드롭(소실).
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;
		APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
		if (!PS || PS->IsExtracted() || PS->IsRaidDead()) continue;

		PS->SetRaidDead(true);
		FinalizeRaidStatsOnDeath(PC);
	}

	UE_LOG(LogPDRaid, Warning, TEXT("[Debug] ForceRaidFail -> EndRaid(false)"));
	EndRaid(false);
}

void APDGameMode::ProcessExtractionForPlayer(APlayerController* PC)
{
	if (!PC) return;

	APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
	if (!PS) return;

	UE_LOG(LogPDRaid, Log, TEXT("ProcessExtraction: PC=%s -> transfer inventory to stash"),
		*PC->GetName());

	// 1. 키트(장비 + 퀵슬롯 보관 아이템) 캡처 → per-player. BuildStashTransferItems 가 이걸 참조해 스태시에서 제외하므로
	//    반드시 TransferPlayerInventoryToStash 보다 먼저 호출.
	PS->CaptureEquippedItemsToPersistent();
	PS->CaptureQuickSlotItemsToPersistent();

	// 2. 키트 제외한 나머지 인벤토리만 공용 스태시로.
	TransferPlayerInventoryToStash(PC);

	// 3. per-player(장비/퀵슬롯 등 PersistentData) 디스크 저장 — 진입 시 재장착/재할당.
	SavePlayerStateToDisk(PC);

	// 4. 공용 스태시는 GI 단일 슬롯에 저장(per-player 아님). GI 는 트래블 너머 호스트에서 유지됨.
	if (UPDGameInstance* GI = GetGameInstance<UPDGameInstance>())
	{
		GI->SaveToDisk();
	}

	// SecureContainer ???�존(추출) ???�음 ?�이?�로 ?�월.
	if (APawn* Pawn = PC->GetPawn())
	{
		if (UPDSecureContainerComponent* SecureContainer = Pawn->FindComponentByClass<UPDSecureContainerComponent>())
		{
			SecureContainer->SaveSecureContainer();
			UE_LOG(LogPDRaid, Log, TEXT("ProcessExtraction: PC=%s saved SecureContainer"), *PC->GetName());
		}
	}
}

void APDGameMode::ProcessDeathForPlayer(APlayerController* PC)
{
	if (!PC) return;

	// Death does not immediately transfer inventory; EndRaid handles persistent raid data.
    UE_LOG(LogPDRaid, Verbose, TEXT("ProcessDeath: PC=%s -> raid inventory will be dropped on EndRaid"),
        *PC->GetName());
}

void APDGameMode::HandleSpectatorTransitionForDeath(APlayerController* DeadPC)
{
	APDPlayerController* DeadPDPC = Cast<APDPlayerController>(DeadPC);
	if (!DeadPDPC) return;

	APlayerController* InitialTarget = FindFirstAliveSpectateTarget(DeadPC);
	if (!InitialTarget)
	{
		UE_LOG(LogPDRaid, Log, TEXT("HandleSpectatorTransitionForDeath: PC=%s no alive target, skip"),
			*DeadPC->GetName());
		return;
	}

	DeadPDPC->StartSpectatingDeath(InitialTarget);
}

void APDGameMode::NotifyOthersWatchingFinalized(APlayerController* AffectedPC)
{
	if (!AffectedPC) return;
	UWorld* World = GetWorld();
	if (!World) return;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APDPlayerController* Other = Cast<APDPlayerController>(It->Get());
		if (!Other || Other == AffectedPC) continue;
		Other->CycleSpectateIfTargetIs(AffectedPC);
	}
}

APlayerController* APDGameMode::FindFirstAliveSpectateTarget(APlayerController* ExcludePC) const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || PC == ExcludePC) continue;

		APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
		if (!PS) continue;
		if (PS->IsRaidDead() || PS->IsExtracted()) continue;
		if (!PC->GetPawn()) continue;

		return PC;
	}
	return nullptr;
}

void APDGameMode::FinalizeStrandedDownedPlayers()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// LifeState 가 진짜 Alive ??캐릭?��? 카운?? Downed ???�로 모음.
	int32 AliveCount = 0;
	TArray<APDCharacterBase*> StrandedDowned;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;
		APDCharacterBase* Char = Cast<APDCharacterBase>(PC->GetPawn());
		if (!Char) continue;

		switch (Char->GetLifeState())
		{
		case EPDLifeState::Alive:
			++AliveCount;
			break;
		case EPDLifeState::Downed:
			StrandedDowned.Add(Char);
			break;
		default:
			break;
		}
	}

	if (AliveCount > 0 || StrandedDowned.Num() == 0) return;

	UE_LOG(LogPDRaid, Log, TEXT("FinalizeStrandedDowned: no Alive remain, forcing %d Downed pawn(s) to die"),
		StrandedDowned.Num());

	// �?HandleDeath 가 OnPlayerDied ??FinalizeStrandedDownedPlayers ?��? 가??
	// ?��? Dead �??�환????��?� LifeState 가?�로 ?�킵??(HandleDeath ?��?).
	for (APDCharacterBase* DownedChar : StrandedDowned)
	{
		if (DownedChar && DownedChar->GetLifeState() == EPDLifeState::Downed)
		{
			DownedChar->HandleDeath(nullptr);
		}
	}
}

float APDGameMode::GetCurrentRaidElapsedSeconds() const
{
	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	const float Now = GS ? GS->GetServerWorldTimeSeconds() : (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
	return FMath::Max(0.f, Now - RaidStartServerTime);
}

int32 APDGameMode::CountInventoryItemQuantity(UPDInventoryComponent* Inventory) const
{
	if (!Inventory) return 0;
	int32 Total = 0;
	for (const FPDInventorySlot& Slot : Inventory->Items)
	{
		if (!Slot.IsEmpty())
		{
			Total += FMath::Max(0, Slot.Quantity);
		}
	}
	return Total;
}

void APDGameMode::CaptureInitialRaidSnapshotFor(APlayerController* PC)
{
	if (!PC) return;
	APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
	if (!PS) return;

	UPDInventoryComponent* Inventory = PS->GetInventoryComponent();
	const int32 InitialGold = Inventory ? Inventory->Gold : 0;
	const int32 InitialItemQty = CountInventoryItemQuantity(Inventory);
	PS->CaptureInitialRaidSnapshot(InitialGold, InitialItemQty);

	UE_LOG(LogPDRaid, Log, TEXT("CaptureInitialRaidSnapshot: PC=%s Gold=%d ItemQty=%d"),
		*PC->GetName(), InitialGold, InitialItemQty);
}

void APDGameMode::FinalizeRaidStatsOnDeath(APlayerController* DeadPC)
{
	if (!DeadPC) return;
	APDPlayerState* PS = DeadPC->GetPlayerState<APDPlayerState>();
	if (!PS) return;

	// ?�망?? ?�벤/골드 모두 ?�실 ??Delta ???�작 ?�냅???�수.
	PS->SetSurvivalSeconds(GetCurrentRaidElapsedSeconds());
	PS->SetGoldDelta(-PS->GetInitialRaidGold());
	PS->SetItemDelta(-PS->GetInitialRaidItemQuantity());

	UE_LOG(LogPDRaid, Log, TEXT("FinalizeRaidStatsOnDeath: PC=%s Survive=%.1fs GoldDelta=%d ItemDelta=%d"),
		*DeadPC->GetName(),
		PS->GetRaidStats().SurvivalSeconds,
		PS->GetRaidStats().GoldDelta,
		PS->GetRaidStats().ItemDelta);
}

void APDGameMode::FinalizeRaidStatsOnExtraction(APlayerController* ExtractedPC)
{
	if (!ExtractedPC) return;
	APDPlayerState* PS = ExtractedPC->GetPlayerState<APDPlayerState>();
	if (!PS) return;

	UPDInventoryComponent* Inventory = PS->GetInventoryComponent();
	const int32 FinalGold = Inventory ? Inventory->Gold : 0;
	const int32 FinalItemQty = CountInventoryItemQuantity(Inventory);

	PS->SetSurvivalSeconds(GetCurrentRaidElapsedSeconds());
	PS->SetGoldDelta(FinalGold - PS->GetInitialRaidGold());
	PS->SetItemDelta(FinalItemQty - PS->GetInitialRaidItemQuantity());

	UE_LOG(LogPDRaid, Log, TEXT("FinalizeRaidStatsOnExtraction: PC=%s Survive=%.1fs GoldDelta=%d ItemDelta=%d"),
		*ExtractedPC->GetName(),
		PS->GetRaidStats().SurvivalSeconds,
		PS->GetRaidStats().GoldDelta,
		PS->GetRaidStats().ItemDelta);
}

void APDGameMode::CreditKillToShooter(AActor* Killer, APlayerController* Victim)
{
	if (!Killer || !Victim) return;

	// Killer ??보통 공격?�의 Pawn. PlayerController �??�원.
	APawn* KillerPawn = Cast<APawn>(Killer);
	APlayerController* KillerPC = KillerPawn ? Cast<APlayerController>(KillerPawn->GetController()) : Cast<APlayerController>(Killer);
	if (!KillerPC || KillerPC == Victim) return;

	APDPlayerState* KillerPS = KillerPC->GetPlayerState<APDPlayerState>();
	if (!KillerPS) return;

	KillerPS->AddKill();
	UE_LOG(LogPDRaid, Log, TEXT("CreditKillToShooter: Killer=%s Victim=%s Kills=%d"),
		*KillerPC->GetName(), *Victim->GetName(), KillerPS->GetRaidStats().Kills);
}

void APDGameMode::BuildRaidEndEntries(TArray<FPDPlayerRaidEntryData>& OutEntries) const
{
	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	if (!GS) return;

	OutEntries.Reserve(GS->PlayerArray.Num());
	for (APlayerState* PS : GS->PlayerArray)
	{
		if (!PS) continue;

		FPDPlayerRaidEntryData Data;
		Data.PlayerName = PS->GetPlayerName();

		if (const APDPlayerState* PDPS = Cast<APDPlayerState>(PS))
		{
			Data.bSurvived = PDPS->IsExtracted();
			Data.Stats     = PDPS->GetRaidStats();
		}
		else
		{
			Data.bSurvived = false;
			Data.Stats     = FPDRaidStats{};
		}
		OutEntries.Add(MoveTemp(Data));
	}
}

void APDGameMode::SetRaidState(ERaidState NewState)
{
	if (CurrentRaidState==NewState) return;

	UE_LOG(LogPDRaid, Log, TEXT("SetRaidState: %d -> %d"),
		static_cast<int32>(CurrentRaidState), static_cast<int32>(NewState));

	CurrentRaidState=NewState;
	if (APDGameState* GS=GetGameState<APDGameState>()) GS->SetRaidState(NewState);
	OnRaidStateChanged(NewState);
}


void APDGameMode::InitializePlayerStateFromSave(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	APDPlayerState* PDPlayerState = PC->GetPlayerState<APDPlayerState>();
	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!PDPlayerState || !GI)
	{
		return;
	}

	// 로드/세이브 키가 안정적이도록 최초 1회 SaveId 보장 (CopyProperties 로 트래블 너머 유지됨).
	PDPlayerState->EnsurePersistentSaveId();

	const FString LoadKey = GI->GetSaveKeyForController(PC);
	const FPDPlayerData Loaded = GI->LoadPlayerDataFromDisk(LoadKey, PC->IsLocalController());
	PDPlayerState->InitializePersistentData(Loaded);

	// per-player 키트 복원. PostLogin 에서 Super 가 이미 폰을 소유시킨 뒤 이 함수가 불리므로 폰이 준비됨.
	// (없으면 no-op, 이미 적용돼 있으면 skip — idempotent.)
	// 베이스에서는 인벤토리 리셋이 없어 그대로 유지. 레이드는 StartRaid 의 로드아웃 리셋 후 재복원됨.
	PDPlayerState->RestoreEquippedItemsFromPersistent();
	PDPlayerState->RestoreQuickSlotItemsFromPersistent();
}

void APDGameMode::SavePlayerStateToDisk(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	APDPlayerState* PDPlayerState = PC->GetPlayerState<APDPlayerState>();
	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!PDPlayerState || !GI)
	{
		return;
	}

	const FString SaveKey = GI->GetSaveKeyForController(PC);
	const FPDPlayerData& Data = PDPlayerState->GetPersistentData();
	GI->SavePlayerDataToDisk(SaveKey, Data);
}

void APDGameMode::InitializePlayerInventoryFromLoadout(APlayerController* PC)
{
	if (!PC) return;

	APDPlayerState* PDPlayerState = PC->GetPlayerState<APDPlayerState>();
	UPDInventoryComponent* Inventory = PDPlayerState ? PDPlayerState->GetInventoryComponent() : nullptr;
	if (!Inventory) return;

	Inventory->ResetInventory();

	for (const FPDInventorySlot& Slot : PDPlayerState->GetRaidLoadout())
	{
		if (!Slot.IsEmpty())
		{
			Inventory->AddItem(Slot.ItemData, Slot.Quantity);
		}
	}

	Inventory->Gold = PDPlayerState->GetRaidGold();
	PDPlayerState->ClearRaidLoadout();
}

void APDGameMode::TransferPlayerInventoryToStash(APlayerController* PC)
{
	if (!PC) return;

	APDPlayerState* PDPlayerState = PC->GetPlayerState<APDPlayerState>();
	UPDInventoryComponent* Inventory = PDPlayerState ? PDPlayerState->GetInventoryComponent() : nullptr;
	if (!Inventory) return;

	UPDGameInstance* GI = GetGameInstance<UPDGameInstance>();
	if (!GI) return;

	// 키트(퀵슬롯 보관 + 장착 장비) 제외한 나머지 인벤토리만 공용 스태시로.
	// 장착 장비는 인벤토리에 없어 자연 제외, 퀵슬롯 보관분은 BuildStashTransferItems 가 제외.
	// 선행조건: ProcessExtractionForPlayer 에서 CaptureEquipped/CaptureQuickSlot 이 먼저 호출됨.
	TArray<FPDInventorySlot> ToStash;
	PDPlayerState->BuildStashTransferItems(ToStash);
	GI->MergeSlotsIntoStash(ToStash, Inventory->Gold);
}

void APDGameMode::NotifyPlayerReadyForTravel(APlayerController* PC)
{
	if (!PC || bTravelStarted) return;

	// 결산 ?�젯???�기 ?�에 ?�릭?�로 ?�어?�는 ACK 차단. EndRaid 가 ?�출?�어 Ended ?�태???�만 받음.
	if (CurrentRaidState != ERaidState::Ended)
	{
		UE_LOG(LogPDRaid, Log, TEXT("NotifyPlayerReadyForTravel: ignored, raid not Ended (state=%d) PC=%s"),
			static_cast<int32>(CurrentRaidState), *PC->GetName());
		return;
	}

	const bool bWasAdded = ReadyPlayersForTravel.AddUnique(PC) != INDEX_NONE;
	const int32 ReadyCount = ReadyPlayersForTravel.Num();
	const int32 TotalPlayers = GetGameState<AGameStateBase>() ? GetGameState<AGameStateBase>()->PlayerArray.Num() : 0;

	// PS �?ACK ?�래�??�팅 ??모든 ?�라가 ?�젯?�로 ?��? ACK ?�는지 ?�시 가??
	if (APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>())
	{
		PS->SetTravelReady(true);
	}

	UE_LOG(LogPDRaid, Log, TEXT("NotifyPlayerReadyForTravel: PC=%s ack=%d/%d (newAck=%d)"),
		*PC->GetName(), ReadyCount, TotalPlayers, bWasAdded ? 1 : 0);

	if (AreAllPlayersReadyForTravel())
	{
		UE_LOG(LogPDRaid, Log, TEXT("NotifyPlayerReadyForTravel: all players ACK'd -> RequestBaseTravel"));
		RequestBaseTravel();
	}
}

bool APDGameMode::AreAllPlayersReadyForTravel() const
{
	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	if (!GS) return false;

	const int32 TotalPlayers = GS->PlayerArray.Num();
	if (TotalPlayers <= 0) return false;

	int32 ValidReadyCount = 0;
	for (const TWeakObjectPtr<APlayerController>& WeakPC : ReadyPlayersForTravel)
	{
		if (WeakPC.IsValid()) ++ValidReadyCount;
	}
	return ValidReadyCount >= TotalPlayers;
}

void APDGameMode::OnTravelTimeoutExpired()
{
	if (bTravelStarted) return;
	UE_LOG(LogPDRaid, Warning, TEXT("OnTravelTimeoutExpired: %.1fs gate expired, forcing ServerTravel"),
		TravelGateTimeoutSeconds);
	RequestBaseTravel();
}

void APDGameMode::RequestBaseTravel()
{
	if (bTravelStarted)
	{
		UE_LOG(LogPDRaid, Log, TEXT("RequestBaseTravel: already started, ignored"));
		return;
	}
	bTravelStarted = true;

	GetWorldTimerManager().ClearTimer(TravelTimeoutTimerHandle);

	UE_LOG(LogPDRaid, Log, TEXT("RequestBaseTravel: invoking seamless ServerTravel to BaseLevel"));

	if (UPDGameInstance* GI = GetGameInstance<UPDGameInstance>())
	{
		GI->TravelToLevel(GI->GetBaseLevel(), /*bMarkBaseResetPending=*/false);
	}
}

int32 APDGameMode::CountAlivePlayers() const
{
	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	if (!GS) return 0;

	int32 Count = 0;
	for (APlayerState* PSBase : GS->PlayerArray)
	{
		const APDPlayerState* PS = Cast<APDPlayerState>(PSBase);
		if (PS && !PS->IsRaidDead() && !PS->IsExtracted()) ++Count;
	}
	return Count;
}

int32 APDGameMode::CountExtractedPlayers() const
{
	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	if (!GS) return 0;

	int32 Count = 0;
	for (APlayerState* PSBase : GS->PlayerArray)
	{
		const APDPlayerState* PS = Cast<APDPlayerState>(PSBase);
		if (PS && PS->IsExtracted()) ++Count;
	}
	return Count;
}

int32 APDGameMode::CountDeadPlayers() const
{
	const AGameStateBase* GS = GetGameState<AGameStateBase>();
	if (!GS) return 0;

	int32 Count = 0;
	for (APlayerState* PSBase : GS->PlayerArray)
	{
		const APDPlayerState* PS = Cast<APDPlayerState>(PSBase);
		if (PS && PS->IsRaidDead()) ++Count;
	}
	return Count;
}

APDGameState* APDGameMode::GetPDGameState() const
{
	return GetGameState<APDGameState>();
}

void APDGameMode::NotifyZoneOccupancy(EPDZoneTravelType ZoneType, APlayerController* PC, bool bEntered, TSoftObjectPtr<UWorld> RaidLevel)
{
	UE_LOG(LogTemp, Warning, TEXT("[Diag-Zone] NotifyZoneOccupancy: Type=%d PC=%s Entered=%d HasAuth=%d"),
		static_cast<int32>(ZoneType), *GetNameSafe(PC), bEntered ? 1 : 0, HasAuthority() ? 1 : 0);

	if (!HasAuthority() || !PC || ZoneType == EPDZoneTravelType::None) return;

	if (bEntered)
	{
		ZoneOccupants.Add(PC);
		CurrentZoneType = ZoneType;
		if (ZoneType == EPDZoneTravelType::RaidEntry && !RaidLevel.IsNull())
		{
			PendingRaidLevel = RaidLevel;
		}
	}
	else
	{
		ZoneOccupants.Remove(PC);
	}

	EvaluateZoneCountdown();
}

void APDGameMode::EvaluateZoneCountdown()
{
	APDGameState* GS = GetPDGameState();
	UWorld* World = GetWorld();
	if (!GS || !World) return;

	// 무효(파괴/로그아웃) 점유자 정리 + 유효 인원 카운트.
	int32 InZone = 0;
	for (auto It = ZoneOccupants.CreateIterator(); It; ++It)
	{
		if (It->IsValid()) { ++InZone; }
		else { It.RemoveCurrent(); }
	}

	// 참가자 총원: Extraction 은 살아있는 인원, RaidEntry(Base) 는 접속 인원.
	int32 Total = 0;
	if (CurrentZoneType == EPDZoneTravelType::Extraction)
	{
		Total = CountAlivePlayers();
	}
	else
	{
		for (APlayerState* PSBase : GS->PlayerArray)
		{
			if (Cast<APDPlayerState>(PSBase)) { ++Total; }
		}
	}

	const bool bThresholdMet = Total > 0 && InZone * 2 >= Total;
	const bool bAllIn = Total > 0 && InZone >= Total;

	UE_LOG(LogTemp, Warning, TEXT("[Diag-Zone] EvaluateZoneCountdown: Type=%d InZone=%d Total=%d ThresholdMet=%d AllIn=%d"),
		static_cast<int32>(CurrentZoneType), InZone, Total, bThresholdMet ? 1 : 0, bAllIn ? 1 : 0);

	FTimerManager& TM = World->GetTimerManager();

	if (!bThresholdMet)
	{
		// 절반 미만 → 취소.
		TM.ClearTimer(ZoneCountdownTimerHandle);
		CurrentZoneType = EPDZoneTravelType::None;
		bZoneFinalCountdownActive = false;
		ZoneEndServerTime = -1.f;
		GS->ClearZoneCountdown();
		return;
	}

	const float ServerNow = GS->GetServerWorldTimeSeconds();
	const bool bAlreadyCounting = TM.IsTimerActive(ZoneCountdownTimerHandle);

	if (!bAlreadyCounting)
	{
		// 절반 이상 최초 도달 → 20초 카운트 시작.
		bZoneFinalCountdownActive = false;
		ZoneEndServerTime = ServerNow + ZoneCountdownSeconds;
		TM.SetTimer(ZoneCountdownTimerHandle, this, &APDGameMode::OnZoneCountdownFired, ZoneCountdownSeconds, false);
	}
	else if (bAllIn && !bZoneFinalCountdownActive)
	{
		// 전원 진입 → 3초로 즉시 단축.
		bZoneFinalCountdownActive = true;
		ZoneEndServerTime = ServerNow + ZoneFinalCountdownSeconds;
		TM.SetTimer(ZoneCountdownTimerHandle, this, &APDGameMode::OnZoneCountdownFired, ZoneFinalCountdownSeconds, false);
	}
	// else: 진행 중 + 인원만 변동 → 타이머 유지, 표시 인원만 갱신.

	UE_LOG(LogTemp, Warning, TEXT("[Diag-Zone] -> GS->SetZoneCountdown: Type=%d InZone=%d Total=%d EndServerTime=%.1f Final=%d (ServerNow=%.1f)"),
		static_cast<int32>(CurrentZoneType), InZone, Total, ZoneEndServerTime, bZoneFinalCountdownActive ? 1 : 0, ServerNow);

	GS->SetZoneCountdown(CurrentZoneType, InZone, Total, ZoneEndServerTime, bZoneFinalCountdownActive);
}

void APDGameMode::OnZoneCountdownFired()
{
	const EPDZoneTravelType FiredType = CurrentZoneType;

	// 발동 시점 점유 인원 스냅샷(추출 대상).
	TArray<APlayerController*> Occupants;
	for (const TWeakObjectPtr<APlayerController>& Weak : ZoneOccupants)
	{
		if (APlayerController* PC = Weak.Get()) { Occupants.Add(PC); }
	}

	// 상태 정리(위젯 숨김).
	ZoneOccupants.Empty();
	CurrentZoneType = EPDZoneTravelType::None;
	bZoneFinalCountdownActive = false;
	ZoneEndServerTime = -1.f;
	if (APDGameState* GS = GetPDGameState())
	{
		GS->ClearZoneCountdown();
	}

	if (FiredType == EPDZoneTravelType::RaidEntry)
	{
		// 레벨 에셋으로 트래블(로딩스크린 + 1프레임 지연 ServerTravel 은 GI 가 처리).
		if (UPDGameInstance* GI = GetGameInstance<UPDGameInstance>())
		{
			GI->TravelToLevel(PendingRaidLevel, /*bMarkBaseResetPending*/ false);
		}
	}
	else if (FiredType == EPDZoneTravelType::Extraction)
	{
		// 존 안 인원: 추출 성공(아이템 유지).
		for (APlayerController* PC : Occupants)
		{
			RequestExtraction(PC);
		}

		// 존 밖 생존자: 추출 실패(사망 마감, 아이템 손실). 전원 마감시켜야 EndRaid 가 트래블함.
		if (UWorld* World = GetWorld())
		{
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* PC = It->Get();
				if (!PC) continue;
				APDPlayerState* PS = PC->GetPlayerState<APDPlayerState>();
				if (!PS || PS->IsExtracted() || PS->IsRaidDead()) continue;

				PS->SetRaidDead(true);
				FinalizeRaidStatsOnDeath(PC);
			}
		}

		// 전원 마감 → EndRaid(1명이라도 추출 시 성공) → 결산 위젯 → Base 트래블.
		EvaluateRaidEnd();
	}
}
