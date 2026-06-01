#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDEquipmentModificationDisplayActor.generated.h"

class APDEquipmentModificationActor;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDEquipmentModificationDisplayActor : public AActor
{
	GENERATED_BODY()

public:
	APDEquipmentModificationDisplayActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification|Display")
	void TurnOnDisplay();

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification|Display")
	void TurnOffDisplay();

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification|Display")
	void BindToEquipmentModificationActor();

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification|Display")
	void UnbindFromEquipmentModificationActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Display")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Display")
	TObjectPtr<UStaticMeshComponent> DisplayMesh;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "PD|Equipment Modification|Display")
	TObjectPtr<APDEquipmentModificationActor> TargetEquipmentModificationActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Display")
	TObjectPtr<UMaterialInterface> OffMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Display")
	TObjectPtr<UMaterialInterface> OnMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Display")
	FName TargetMeshComponentName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification|Display")
	int32 MaterialElementIndex = 0;

private:
	UStaticMeshComponent* GetTargetMeshComponent() const;
	void ApplyDisplayMaterial(UMaterialInterface* Material);

	UFUNCTION()
	void HandleEquipmentModificationOpened(APDEquipmentModificationActor* ModificationActor);

	UFUNCTION()
	void HandleEquipmentModificationClosed(APDEquipmentModificationActor* ModificationActor);
};
