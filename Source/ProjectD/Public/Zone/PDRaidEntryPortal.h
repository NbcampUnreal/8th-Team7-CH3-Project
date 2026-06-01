#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDRaidEntryPortal.generated.h"

class UBoxComponent;
class UWorld;

UCLASS(Blueprintable)
class PROJECTD_API APDRaidEntryPortal : public AActor
{
	GENERATED_BODY()

public:
	APDRaidEntryPortal();

protected:
	UPROPERTY(VisibleAnywhere, Category="PD|Portal")
	TObjectPtr<UBoxComponent> OverlapVolume;

	// 진입할 레이드 레벨 에셋. 레벨에 배치된 포털에서 직접 할당(맵 이름 문자열 대신).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PD|Portal")
	TSoftObjectPtr<UWorld> RaidLevel;

	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintImplementableEvent, Category="PD|Portal")
	void OnRaidEntryTriggered(APlayerController* PC);

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// 진입 사전 준비(로드아웃 확정 등). 실제 트래블은 존 카운트다운 만료 시 GameMode 가 수행.
	void PrepareEntry(APlayerController* PC);
};
