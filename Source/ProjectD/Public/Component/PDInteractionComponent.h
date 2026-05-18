#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDInteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractTargetChanged, AActor*, NewTarget);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDInteractionComponent();

	UFUNCTION(BlueprintCallable, Category = "PD|Interaction")
	void Interact();

	UFUNCTION(BlueprintCallable, Category = "PD|Interaction")
	AActor* FindInteractTarget() const;

	UFUNCTION(BlueprintCallable, Category = "PD|Interaction")
	AActor* GetCurrentTarget() const { return CachedTarget.Get(); }

	UPROPERTY(BlueprintAssignable, Category = "PD|Interaction")
	FOnInteractTargetChanged OnInteractTargetChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Interaction")
	float InteractDistance = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Interaction")
	float PollInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Interaction", meta = (ClampMin = "0.0"))
	float OverlapProbeRadius = 50.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void PollTarget();
	AActor* ChooseClosestInteractable(const TArray<AActor*>& Candidates, const FVector& FromLocation) const;

	TWeakObjectPtr<AActor> CachedTarget;
	FTimerHandle PollTimerHandle;
};