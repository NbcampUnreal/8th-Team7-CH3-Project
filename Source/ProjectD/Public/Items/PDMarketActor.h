#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/PDInteractable.h"
#include "PDMarketActor.generated.h"

class UBoxComponent;
class UPDInteractionOutlineComponent;
class UPDMarketComponent;
class APDMarketActor;
class APDPlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDMarketStateChangedSignature, APDMarketActor*, MarketActor);

UCLASS(Blueprintable)
class PROJECTD_API APDMarketActor : public AActor, public IPDInteractable
{
	GENERATED_BODY()

public:
	APDMarketActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "PD|Market")
	UPDMarketComponent* GetMarketComponent() const { return MarketComponent; }

	UPROPERTY(BlueprintAssignable, Category = "PD|Market|Events")
	FPDMarketStateChangedSignature OnMarketOpened;

	UPROPERTY(BlueprintAssignable, Category = "PD|Market|Events")
	FPDMarketStateChangedSignature OnMarketClosed;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Market")
	TObjectPtr<UBoxComponent> InteractionCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Market")
	TObjectPtr<UPDMarketComponent> MarketComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Market")
	TObjectPtr<UPDInteractionOutlineComponent> OutlineComponent;
private:
	void ConfigureInteractionCollision() const;
	void BindMarketClose(APDPlayerController* PlayerController);
	void UnbindMarketClose();
	void HandleMarketInterfaceClosed(UPDMarketComponent* ClosedMarketComponent);

	TWeakObjectPtr<APDPlayerController> BoundPlayerController;
};
