#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDStashDisplayActor.generated.h"

class APDStashActor;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDStashDisplayActor : public AActor
{
	GENERATED_BODY()

public:
	APDStashDisplayActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "PD|Stash|Display")
	void TurnOnDisplay();

	UFUNCTION(BlueprintCallable, Category = "PD|Stash|Display")
	void TurnOffDisplay();

	UFUNCTION(BlueprintCallable, Category = "PD|Stash|Display")
	void BindToStashActor();

	UFUNCTION(BlueprintCallable, Category = "PD|Stash|Display")
	void UnbindFromStashActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Stash|Display")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Stash|Display")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Stash|Display")
	TObjectPtr<UStaticMeshComponent> ScreenMesh;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "PD|Stash|Display")
	TObjectPtr<APDStashActor> TargetStashActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Display")
	TObjectPtr<UMaterialInterface> OffMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Display")
	TObjectPtr<UMaterialInterface> OnMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Stash|Display")
	int32 ScreenMaterialElementIndex = 0;

private:
	void ApplyDisplayMaterial(UMaterialInterface* Material);

	UFUNCTION()
	void HandleStashOpened(APDStashActor* StashActor);

	UFUNCTION()
	void HandleStashClosed(APDStashActor* StashActor);
};
