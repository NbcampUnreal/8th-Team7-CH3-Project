// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Type/Types.h"
#include "PDGameState.generated.h"

UCLASS()
class PROJECTD_API APDGameState : public AGameState
{
	GENERATED_BODY()
public:
	void SetRaidState(ERaidState NewState);
	FORCEINLINE ERaidState GetRaidState() const{return CurrentRaidState;}
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PD|Raid")
	ERaidState CurrentRaidState=ERaidState::Idle;
	
	UFUNCTION(BlueprintImplementableEvent, Category="PD|Raid")
	void OnRaidStateChanged(ERaidState NewState);
};
