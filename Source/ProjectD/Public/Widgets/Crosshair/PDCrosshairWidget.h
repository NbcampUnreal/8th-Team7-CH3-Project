// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Type/Types.h"
#include "PDCrosshairWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTD_API UPDCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    // 매 틱 PDPlayerController에서 호출
    
    UFUNCTION(BlueprintCallable, Category = "PD|Crosshair")
    void UpdateCrosshair(FVector2D MousePos, float Spread);

    // 무기 교체 시 호출
    UFUNCTION(BlueprintCallable, Category = "PD|Crosshair")
    void SetCrosshairType(EWeaponType NewType);

    UFUNCTION(BlueprintPure, Category = "PD|Crosshair")
    FORCEINLINE EWeaponType GetCrosshairType() const { return CurrentType; }

    UFUNCTION(BlueprintPure, Category = "PD|Crosshair")
    FORCEINLINE float GetCurrentSpread() const { return CurrentSpread; }

protected:
    // BP 구현 — 스프레드 변화 시 크기 애니메이션
    UFUNCTION(BlueprintImplementableEvent, Category = "PD|Crosshair")
    void OnSpreadChanged(float NewSpread);

    // BP 구현 — 타입 변화 시 모양 전환
    UFUNCTION(BlueprintImplementableEvent, Category = "PD|Crosshair")
    void OnCrosshairTypeChanged(EWeaponType NewType);

    //스프레드 → 크기 변환 배율 (BP에서 무기별로 조정)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Crosshair")
    float SpreadToSizeScale = 10.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Crosshair")
    float MinSize = 20.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Crosshair")
    float MaxSize = 80.f;

private:
    EWeaponType CurrentType = EWeaponType::Rifle;
    float CurrentSpread = 0.f;
};
