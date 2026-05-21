#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDMarketDisplayActor.generated.h"

class APDMarketActor;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDMarketDisplayActor : public AActor
{
	GENERATED_BODY()

public:
	APDMarketDisplayActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "PD|Market|Display")
	void TurnOnDisplay();

	UFUNCTION(BlueprintCallable, Category = "PD|Market|Display")
	void TurnOffDisplay();

	UFUNCTION(BlueprintCallable, Category = "PD|Market|Display")
	void BindToMarketActor();

	UFUNCTION(BlueprintCallable, Category = "PD|Market|Display")
	void UnbindFromMarketActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Market|Display")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Market|Display")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Market|Display")
	TObjectPtr<UStaticMeshComponent> ScreenMesh;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "PD|Market|Display")
	TObjectPtr<APDMarketActor> TargetMarketActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Market|Display")
	TObjectPtr<UMaterialInterface> OffMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Market|Display")
	TObjectPtr<UMaterialInterface> OnMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Market|Display")
	int32 ScreenMaterialElementIndex = 0;

private:
	void ApplyDisplayMaterial(UMaterialInterface* Material);

	UFUNCTION()
	void HandleMarketOpened(APDMarketActor* MarketActor);

	UFUNCTION()
	void HandleMarketClosed(APDMarketActor* MarketActor);
};
