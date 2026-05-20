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