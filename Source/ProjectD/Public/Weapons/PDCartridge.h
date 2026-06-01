#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDCartridge.generated.h"

UCLASS(Blueprintable)
class PROJECTD_API APDCartridge : public AActor
{
	GENERATED_BODY()

public:
	APDCartridge();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category="PD|Cartridge")
	TObjectPtr<UStaticMeshComponent> CartridgeMesh;

	UPROPERTY(EditDefaultsOnly, Category="PD|Cartridge")
	TObjectPtr<USoundBase> FallSound;

	/** 탄피에 가하는 초기 충격량. BP에서 조정. */
	UPROPERTY(EditDefaultsOnly, Category="PD|Cartridge", meta=(ClampMin="0.0"))
	float EjectImpulse = 500.f;

	/** 바닥 충돌 후 자동 제거까지 대기 시간. */
	UPROPERTY(EditDefaultsOnly, Category="PD|Cartridge", meta=(ClampMin="0.0"))
	float DestroyDelay = 5.f;

	FTimerHandle DestroyTimerHandle;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
