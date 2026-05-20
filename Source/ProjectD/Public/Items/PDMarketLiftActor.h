#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDMarketLiftActor.generated.h"

class APDMarketActor;
class USceneComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDMarketLiftActor : public AActor
{
	GENERATED_BODY()

public:
	APDMarketLiftActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "PD|Market|Lift")
	void MoveUp(bool bInstant = false);

	UFUNCTION(BlueprintCallable, Category = "PD|Market|Lift")
	void MoveDown(bool bInstant = false);

	UFUNCTION(BlueprintCallable, Category = "PD|Market|Lift")
	void BindToMarketActor();

	UFUNCTION(BlueprintCallable, Category = "PD|Market|Lift")
	void UnbindFromMarketActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Market|Lift")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "PD|Market|Lift")
	TObjectPtr<APDMarketActor> TargetMarketActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Market|Lift")
	FVector OpenLocationOffset = FVector(0.f, 0.f, 100.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Market|Lift", meta = (ClampMin = "0.01"))
	float MoveInterpSpeed = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Market|Lift", meta = (ClampMin = "0.0"))
	float OpenDelay = 0.f;

private:
	void MoveToTargetLocation(bool bInstant);

	UFUNCTION()
	void HandleMarketOpened(APDMarketActor* MarketActor);

	UFUNCTION()
	void HandleMarketClosed(APDMarketActor* MarketActor);

	FVector ClosedLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	FTimerHandle OpenDelayTimerHandle;
};
