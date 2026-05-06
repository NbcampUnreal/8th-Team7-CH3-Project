#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/PDCharacterBase.h"
#include "Interfaces/PDDetectable.h"
#include "Type/Types.h"
#include "PDEnemyBase.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDEnemyBase : public APDCharacterBase, public IPDDetectable
{
	GENERATED_BODY()

public:
	APDEnemyBase();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|AI")
	EPDEnemyState CurrentState = EPDEnemyState::Idle;


public:
	UFUNCTION(BlueprintCallable, Category = "PD|AI")
	void SetEnemyState(EPDEnemyState NewState);

	FORCEINLINE EPDEnemyState GetEnemyState() const { return CurrentState; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|AI")
	void OnEnemyStateChanged(EPDEnemyState NewState);

	virtual void HandleDeath(AActor* Killer) override;
	virtual void OnVisionExposureChanged_Implementation(AActor* Observer, float Exposure) override;
};
