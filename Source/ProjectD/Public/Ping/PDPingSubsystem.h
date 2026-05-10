#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PDPingTypes.h"
#include "PDPingSubsystem.generated.h"

class APDPingMarker;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPingAdded, const FPDPingData&, PingData); //핑 추가 시
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPingRemoved, int32, PingId); //핑 제거 시

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

	UPROPERTY(BlueprintAssignable, Category="Ping")
	FOnPingAdded OnPingAdded;

	UPROPERTY(BlueprintAssignable, Category="Ping")
	FOnPingRemoved OnPingRemoved;

	UPROPERTY(EditDefaultsOnly, Category="Ping")
	TSoftClassPtr<APDPingMarker> DefaultMarkerClass;
	
	//핑 자동 만료 시간(초).
	UPROPERTY(EditDefaultsOnly, Category="Ping")
	float DefaultPingLifetime = 5.f;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	void ClearAllPings(); //월드 종료 시 핑 전부 제거
	void TickExpiration(); //만료된 핑 제거

	UPROPERTY()
	TMap<int32, FPDPingData> ActivePings;

	int32 NextPingId = 0;

	FTimerHandle ExpireTimerHandle;
};