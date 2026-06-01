#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/PDInteractable.h"
#include "PDInteractableActorBase.generated.h"

class APDPlayerController;
class UBoxComponent;
class UPDInteractionOutlineComponent;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDInteractableActorBase : public AActor, public IPDInteractable
{
	GENERATED_BODY()

public:
	APDInteractableActorBase();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Interaction")
	TObjectPtr<UBoxComponent> InteractionCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Interaction")
	TObjectPtr<UPDInteractionOutlineComponent> OutlineComponent;

	void ConfigureInteractionCollision() const;
	void RefreshOutlineBinding() const;
	APDPlayerController* ResolveInteractorController(AActor* Interactor) const;
};
