#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDPingInputComponent.generated.h"

class UPDInputComponent;
class UPDInputConfig;
class UPDPingSubsystem;
class UPDPingWheelBase;
class APDPingMarker;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTD_API UPDPingInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPDPingInputComponent();
	
	//Controller가 호출
	void BindInputs(UPDInputComponent* PDIC, const UPDInputConfig* InputConfig);
	
	UPROPERTY(EditDefaultsOnly, Category="Ping")
	TSubclassOf<UPDPingWheelBase> PingWheelClass;
	
	UPROPERTY(EditDefaultsOnly, Category="Ping")
	TSubclassOf<APDPingMarker> PingMarkerClass;
	
	UPROPERTY(EditDefaultsOnly, Category="Ping", meta=(ClampMin="0.0"))
	float WheelActivationDelay = 0.15f;
	
protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	//IA_Ping(G)
	void OnPingStarted();
	void OnPingCompleted();
	
	//IA_PingConfirm(LMB)
	void OnPingConfirmStarted();
	void OnPingConfirmCompleted();
	void OnPingConfirmTriggered();
	
	APlayerController* GetOwningPC() const;
	UPDPingSubsystem* GetPingSubsystem() const;
	bool GetMouseScreenPos(FVector2D& OutPos) const;
	bool GetCursorImpactPoint(FVector& OutLocation) const;
	
	UPROPERTY(Transient)
	TObjectPtr<UPDPingWheelBase> PingWheelInstance;

	FVector PingTargetLocation = FVector::ZeroVector;
	FVector2D PingWheelScreenCenter = FVector2D::ZeroVector;
	double ClickStartTime = 0.0;
	bool bIsGPressed = false;
};
