#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDEquipDisplayActor.generated.h"

class APDEquipmentModificationActor;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDEquipDisplayActor : public AActor
{
	GENERATED_BODY()

public:
	APDEquipDisplayActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "PD|Equip|Display")
	void TurnOnDisplay();

	UFUNCTION(BlueprintCallable, Category = "PD|Equip|Display")
	void TurnOffDisplay();

	UFUNCTION(BlueprintCallable, Category = "PD|Equip|Display")
	void BindToEquipActor();

	UFUNCTION(BlueprintCallable, Category = "PD|Equip|Display")
	void UnbindFromEquipActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Equip|Display")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Equip|Display")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Equip|Display")
	TObjectPtr<UStaticMeshComponent> ScreenMesh;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "PD|Equip|Display")
	TObjectPtr<APDEquipmentModificationActor> TargetEquipActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equip|Display")
	TObjectPtr<UMaterialInterface> OffMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equip|Display")
	TObjectPtr<UMaterialInterface> OnMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Equip|Display")
	int32 ScreenMaterialElementIndex = 0;

private:
	void ApplyDisplayMaterial(UMaterialInterface* Material);

	UFUNCTION()
	void HandleEquipOpened(APDEquipmentModificationActor* EquipActor);

	UFUNCTION()
	void HandleEquipClosed(APDEquipmentModificationActor* EquipActor);
};
