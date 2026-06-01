#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDPingTypes.h"
#include "PDPingMarker.generated.h"

class UStaticMeshComponent;

//월드에 보이는 핑 마커 액터
UCLASS(Blueprintable)
class PROJECTD_API APDPingMarker : public AActor
{
	GENERATED_BODY()

public:
	APDPingMarker();
	
	//Subsystem이 스폰 직후 호출.
	UFUNCTION(BlueprintCallable, Category="Ping")
	void InitializePing(EPDPingType InPingType);

protected:
	//BP에서 구현
	UFUNCTION(BlueprintImplementableEvent, Category="Ping")
	void OnPingInitialized(EPDPingType InPingType);
	
	//비주얼 베이스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;
};