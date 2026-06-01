#pragma once

#include "CoreMinimal.h"
#include "PDPingTypes.generated.h"

class APDPingMarker;

//의사소통 + 적 표시 용도.
UENUM(BlueprintType)
enum class EPDPingType : uint8
{
	Default     UMETA(DisplayName="Default"),
	Enemy       UMETA(DisplayName="Enemy"),
	MoveHere    UMETA(DisplayName="Move Here"),
	Item        UMETA(DisplayName="Item"),
	Danger      UMETA(DisplayName="Danger"),
	Help     UMETA(DisplayName="Help"),
};


USTRUCT(BlueprintType)
struct FPDPingData
{
	GENERATED_BODY()
	
	//핑 식별자. RemovePing에 이 값을 넘겨 제거. SpawnPing 시 자동 증가.
	UPROPERTY(BlueprintReadOnly, Category="Ping")
	int32 PingId = -1;

	UPROPERTY(BlueprintReadOnly, Category="Ping")
	EPDPingType PingType = EPDPingType::Default;
	
	UPROPERTY(BlueprintReadOnly, Category="Ping")
	FVector WorldLocation = FVector::ZeroVector;
	
	//월드에 스폰된 비주얼 액터
	UPROPERTY(BlueprintReadOnly, Category="Ping")
	TWeakObjectPtr<APDPingMarker> MarkerActor;
	
	//만료 시점
	UPROPERTY(BlueprintReadOnly, Category="Ping")
	float ExpireTime = 0.f;
};
