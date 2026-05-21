#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDInteractionOutlineComponent.generated.h"

class APawn;
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

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Interaction|Outline")
	int32 StencilValue = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Interaction|Outline")
	bool bOnlyLocalPlayer = true;

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
	void CachePrimitiveRenderState(TArray<UPrimitiveComponent*>& OutComponents) const;
	void ResetOverlapState();

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> TriggerComponent;

	TSet<TWeakObjectPtr<APawn>> OverlappingPawns;
	TMap<TWeakObjectPtr<UPrimitiveComponent>, bool> PreviousCustomDepthStates;
	TMap<TWeakObjectPtr<UPrimitiveComponent>, int32> PreviousStencilValues;
	bool bOutlineEnabled = false;
	bool bTriggerBound = false;
};
