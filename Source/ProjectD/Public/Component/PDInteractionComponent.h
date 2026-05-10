#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDInteractionComponent.generated.h"

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Interaction")
	float InteractDistance = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};
