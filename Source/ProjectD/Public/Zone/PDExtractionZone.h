// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "PDExtractionZone.generated.h"

class UBoxComponent;

UCLASS()
class PROJECTD_API APDExtractionZone : public AActor
{
	GENERATED_BODY()

public:
	APDExtractionZone();

protected:
	UPROPERTY(VisibleAnywhere, Category = "PD|Extraction")
	TObjectPtr<UBoxComponent> OverlapVolume;
	
	UPROPERTY(EditAnywhere, Category = "PD|Extraction")
	float ExtractionDelay=3.f;

	virtual void BeginPlay() override;
	
private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	void TriggerExtraction(APlayerController* PC);
};
