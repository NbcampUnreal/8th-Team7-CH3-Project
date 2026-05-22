// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PDRaidStats.generated.h"

// 레이드 종료 결산 페이로드. 서버 권위로 산출 후 Client RPC로 전달.
USTRUCT(BlueprintType)
struct PROJECTD_API FPDRaidStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "PD|Raid")
	int32 Kills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Raid")
	float SurvivalSeconds = 0.f;

	// 성공 시 +, 실패 시 −. 부호 없이 그대로 표시.
	UPROPERTY(BlueprintReadOnly, Category = "PD|Raid")
	int32 GoldDelta = 0;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Raid")
	int32 ItemDelta = 0;
};

// 결산 위젯이 per-player로 받는 단위. 멀티 환경에서 PC당 1 entry.
// 멀티 머지 후 APDPlayerState 의 Stats / bSurvived 를 그대로 채워 전달.
USTRUCT(BlueprintType)
struct PROJECTD_API FPDPlayerRaidEntryData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "PD|Raid")
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite, Category = "PD|Raid")
	bool bSurvived = false;

	UPROPERTY(BlueprintReadWrite, Category = "PD|Raid")
	FPDRaidStats Stats;
};