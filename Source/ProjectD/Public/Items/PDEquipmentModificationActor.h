#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/PDInteractable.h"
#include "PDEquipmentModificationActor.generated.h"

class UBoxComponent;
class APDPlayerController;
class UPDInteractionOutlineComponent;
class UStaticMeshComponent;

class APDEquipmentModificationActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDEquipmentModificationStateChangedSignature, APDEquipmentModificationActor*, ModificationActor);

UCLASS(Blueprintable)
class PROJECTD_API APDEquipmentModificationActor : public AActor, public IPDInteractable
{
	GENERATED_BODY()

public:
	APDEquipmentModificationActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(BlueprintAssignable, Category = "PD|Equipment Modification|Events")
	FPDEquipmentModificationStateChangedSignature OnEquipmentModificationOpened;

	UPROPERTY(BlueprintAssignable, Category = "PD|Equipment Modification|Events")
	FPDEquipmentModificationStateChangedSignature OnEquipmentModificationClosed;

protected:
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification")
	TObjectPtr<UBoxComponent> InteractionCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification")
	TObjectPtr<UStaticMeshComponent> WorkbenchMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification")
	TObjectPtr<UPDInteractionOutlineComponent> OutlineComponent;

private:
	void ConfigureInteractionCollision() const;
	void BindEquipmentModificationClose(APDPlayerController* PlayerController);
	void UnbindEquipmentModificationClose();
	void HandleEquipmentModificationInterfaceClosed();

	TWeakObjectPtr<APDPlayerController> BoundPlayerController;
};
