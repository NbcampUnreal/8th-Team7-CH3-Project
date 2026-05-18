#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/PDInteractable.h"
#include "PDMarketActor.generated.h"

class UBoxComponent;
class UPDMarketComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDMarketActor : public AActor, public IPDInteractable
{
	GENERATED_BODY()

public:
	APDMarketActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "PD|Market")
	UPDMarketComponent* GetMarketComponent() const { return MarketComponent; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Market")
	TObjectPtr<UBoxComponent> InteractionCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Market")
	TObjectPtr<UPDMarketComponent> MarketComponent;
private:
	void ConfigureInteractionCollision() const;
};
