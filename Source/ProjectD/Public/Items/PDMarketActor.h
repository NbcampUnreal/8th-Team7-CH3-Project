#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/PDInteractable.h"
#include "PDMarketActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UPDMarketComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDMarketActor : public AActor, public IPDInteractable
{
	GENERATED_BODY()

public:
	APDMarketActor();

	virtual void Interact_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "PD|Market")
	UPDMarketComponent* GetMarketComponent() const { return MarketComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Market")
	TObjectPtr<UBoxComponent> InteractionCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Market")
	TObjectPtr<UStaticMeshComponent> MarketMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Market")
	TObjectPtr<UPDMarketComponent> MarketComponent;
};
