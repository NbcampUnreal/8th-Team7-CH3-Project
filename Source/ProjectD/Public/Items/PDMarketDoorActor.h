#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDMarketDoorActor.generated.h"

class APDMarketActor;
class USceneComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDMarketDoorActor : public AActor
{
	GENERATED_BODY()

public:
	APDMarketDoorActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Market|Door")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "PD|Market|Door")
	TObjectPtr<APDMarketActor> TargetMarketActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Market|Door")
	FName UpperDoorComponentName = TEXT("SM_UpperDoor");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Market|Door")
	FName LowerDoorComponentName = TEXT("SM_LowerDoor");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Market|Door")
	FVector UpperOpenOffset = FVector(200.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Market|Door")
	FVector LowerOpenOffset = FVector(-200.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Market|Door", meta = (ClampMin = "0.01"))
	float OpenMoveSpeed = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Market|Door", meta = (ClampMin = "0.01"))
	float CloseMoveSpeed = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Market|Door", meta = (ClampMin = "0.001", ClampMax = "0.5"))
	float FinishAlphaTolerance = 0.001f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Market|Door", meta = (ClampMin = "0.0"))
	float MinimumCloseAlphaSpeed = 0.35f;

private:
	void Bind();
	void Unbind();
	void Apply();

	UFUNCTION()
	void HandleOpened(APDMarketActor* Actor);

	UFUNCTION()
	void HandleClosed(APDMarketActor* Actor);

	TWeakObjectPtr<USceneComponent> UpperDoor;
	TWeakObjectPtr<USceneComponent> LowerDoor;
	FVector UpperClosed = FVector::ZeroVector;
	FVector LowerClosed = FVector::ZeroVector;
	float CurrentAlpha = 0.f;
	float TargetAlpha = 0.f;
};
