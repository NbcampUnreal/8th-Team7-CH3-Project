#pragma once

#include "CoreMinimal.h"
#include "Weapons/PDWeaponBase.h"
#include "PDShotgun.generated.h"


UCLASS(Blueprintable)
class PROJECTD_API APDShotgun : public APDWeaponBase
{
	GENERATED_BODY()
	
public:
    APDShotgun();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Shotgun")
    TArray<int32> PelletCountPerLevel = { 5, 7, 9 };

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Shotgun")
    float SpreadAngle = 15.f;

    // 이 몽타주를 탄 수만큼 반복 재생함
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Shotgun|Animation")
    TObjectPtr<UAnimMontage> ShellInsertMontage;

    // 장전 완료 후 펌프 동작 몽타주
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Shotgun|Animation")
    TObjectPtr<UAnimMontage> PumpMontage;

    // 탄 한 발 삽입 시간 (ShellInsertMontage 길이와 맞춰야 함)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Weapon|Shotgun|Animation")
    float ShellInsertTime = 0.6f;

public:
    virtual void Fire_Implementation() override;
    virtual void Reload_Implementation() override;  // 한 발씩 장전
    virtual void OnShellInserted_Implementation() override; // 탄 삽입 콜백

    UFUNCTION(BlueprintCallable, Category = "PD|Weapon|Shotgun")
    void InterruptReloadAndFire();

private:
    int32 GetCurrentPelletCount() const;
    void PerformPelletTraces(TArray<FHitResult>& OutHits);

    // 한 발씩 반복 장전용
    int32 ShellsToLoad = 0;   // 이번 재장전에서 넣어야 할 총 발 수
    int32 ShellsInserted = 0; // 지금까지 넣은 발 수

    void LoadNextShell();  // 다음 탄 삽입 몽타주 재생 or 장전 완료

    UFUNCTION()
    void OnShellInsertMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};
