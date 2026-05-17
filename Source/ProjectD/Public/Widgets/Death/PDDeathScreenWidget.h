// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDDeathScreenWidget.generated.h"

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDDeathScreenWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	UPDDeathScreenWidget(const FObjectInitializer& ObjectInitializer);
};
