#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDInteractionOutlineComponent.generated.h"

class AActor;
class APawn;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMeshComponent;
class UPrimitiveComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDInteractionOutlineComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDInteractionOutlineComponent();

	void SetupTrigger(UPrimitiveComponent* InTriggerComponent);

	UFUNCTION(BlueprintCallable, Category = "PD|Interaction|Outline")
	void SetOutlineEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "PD|Interaction|Outline")
	bool IsOutlineEnabled() const { return bOutlineEnabled; }

	UFUNCTION(BlueprintCallable, Category = "PD|Interaction|Outline")
	void RefreshOutlineTargets();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Interaction|Outline")
	TObjectPtr<UMaterialInterface> OutlineMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Interaction|Outline", meta = (ClampMin = "0.0"))
	float OutlineIntensity = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Interaction|Outline", meta = (ClampMin = "0.0"))
	float OutlineThickness = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Interaction|Outline")
	bool bOnlyLocalPlayer = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "PD|Interaction|Outline")
	TArray<TObjectPtr<AActor>> OutlineTargetActors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Interaction|Outline")
	bool bApplyToChildActors = false;

private:
	UFUNCTION()
	void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void BindTrigger();
	void UnbindTrigger();
	bool IsValidInteractor(AActor* Actor) const;
	void ApplyOverlayMaterial();
	void CacheMeshComponents(TArray<UMeshComponent*>& OutComponents) const;
	void AppendActorMeshComponents(AActor* Actor, TArray<UMeshComponent*>& OutComponents, TSet<TWeakObjectPtr<UMeshComponent>>& AddedComponents) const;
	void ResetOverlapState();
	void UpdateOutlineParameters();

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> TriggerComponent;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OutlineMID;

	TSet<TWeakObjectPtr<APawn>> OverlappingPawns;
	bool bOutlineEnabled = false;
	bool bTriggerBound = false;
};
