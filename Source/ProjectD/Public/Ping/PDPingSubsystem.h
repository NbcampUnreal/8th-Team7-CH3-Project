#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PDPingTypes.h"
#include "PDPingSubsystem.generated.h"

class APDPingMarker;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPingAdded, const FPDPingData&, PingData); //핑 추가 시
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPingRemoved, int32, PingId); //핑 제거 시

//잔존 표식 데이터 (Enemy 핑 만료 후 남는 옅은 표식)
USTRUCT(BlueprintType)
struct FPDFaintMark
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Ping")
    int32 FaintId = -1;

    UPROPERTY(BlueprintReadOnly, Category="Ping")
    FVector WorldLocation = FVector::ZeroVector;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFaintMarkAdded, const FPDFaintMark&, Mark); //표식 추가 시
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFaintMarkRemoved, int32, FaintId); //표식 제거 시

UCLASS()
class PROJECTD_API UPDPingSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Ping")
    int32 SpawnPing(EPDPingType InType, const FVector& InWorldLocation);

    UFUNCTION(BlueprintCallable, Category="Ping")
    bool RemovePing(int32 InPingId);

    UFUNCTION(BlueprintPure, Category="Ping")
    void GetActivePings(TArray<FPDPingData>& OutPings) const;

    //휠이 열려있거나 활성 핑이 있는지
    UFUNCTION(BlueprintPure, Category="Ping")
    bool IsPingActive() const { return bGPressed || bWheelOpen || ActivePings.Num() > 0; }

    UFUNCTION(BlueprintCallable, Category="Ping")
    void SetWheelOpen(bool bInOpen) { bWheelOpen = bInOpen; }

    UFUNCTION(BlueprintCallable, Category="Ping")
    void SetGPressed(bool bInPressed) { bGPressed = bInPressed; }

    UPROPERTY(BlueprintAssignable, Category="Ping")
    FOnPingAdded OnPingAdded;

    UPROPERTY(BlueprintAssignable, Category="Ping")
    FOnPingRemoved OnPingRemoved;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ping")
    TSubclassOf<APDPingMarker> DefaultMarkerClass;

    //핑 자동 만료 시간
    UPROPERTY(EditDefaultsOnly, Category="Ping")
    float DefaultPingLifetime = 5.f;

    ////-------------------잔존 표식-------------------
    UFUNCTION(BlueprintCallable, Category="Ping")
    int32 AddFaintMark(const FVector& InWorldLocation);

    UFUNCTION(BlueprintCallable, Category="Ping")
    bool RemoveFaintMark(int32 InFaintId);

    //위치 근처 잔존 표식 제거(수동 삭제용)
    UFUNCTION(BlueprintCallable, Category="Ping") //위치 근처 삭제
    bool RemoveFaintMarkNearLocation(const FVector& InWorldLocation, float Radius = 300.f);
    UFUNCTION(BlueprintCallable, Category="Ping") //전체 삭제
    void ClearAllFaintMarks();
    UFUNCTION(BlueprintPure, Category="Ping") //특정 ID 삭제
    void GetActiveFaintMarks(TArray<FPDFaintMark>& OutMarks) const;

    UPROPERTY(BlueprintAssignable, Category="Ping")
    FOnFaintMarkAdded OnFaintMarkAdded;

    UPROPERTY(BlueprintAssignable, Category="Ping")
    FOnFaintMarkRemoved OnFaintMarkRemoved;

    //적 자동 발견 검사 활성화/비활성화
    UFUNCTION(BlueprintCallable, Category="Ping")
    void StartEnemyDetectionCheck(float Interval = 1.0f);

    UFUNCTION(BlueprintCallable, Category="Ping")
    void StopEnemyDetectionCheck();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
    //최대 핑 개수
    static constexpr int32 MaxActivePings = 5;

    //월드 종료 시 핑 전부 제거
    void ClearAllPings();

    //만료된 핑 제거 + Enemy 핑이면 잔존 표식 생성
    void TickExpiration();

    //적이 표식 근처 && 시야 안이면 표식 제거 
    void CheckFaintMarksForEnemy();

    //잔존 표식 검사 반경
    UPROPERTY(EditDefaultsOnly, Category="Ping", meta=(ClampMin="0.0"))
    float FaintMarkDetectRadius = 1000.f;

    bool bWheelOpen = false;
    bool bGPressed = false;

    UPROPERTY()
    TMap<int32, FPDPingData> ActivePings;

    UPROPERTY()
    TMap<int32, FPDFaintMark> ActiveFaintMarks;

    int32 NextPingId = 0;
    int32 NextFaintId = 0;

    FTimerHandle ExpireTimerHandle;
    FTimerHandle FaintMarkCheckTimer;
};