#pragma once

#include "CoreMinimal.h"
#include "Items/Station/PDStationActorBase.h"
#include "PDMarketActor.generated.h"

class UPDMarketComponent;
class APDMarketActor;
class APDPlayerController;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDMarketStateChangedSignature, APDMarketActor*, MarketActor);

UCLASS(Blueprintable)
class PROJECTD_API APDMarketActor : public APDStationActorBase
{
	GENERATED_BODY()

public:
	APDMarketActor();

	UFUNCTION(BlueprintPure, Category = "PD|Market")
	UPDMarketComponent* GetMarketComponent() const { return MarketComponent; }

	UPROPERTY(BlueprintAssignable, Category = "PD|Market|Events")
	FPDMarketStateChangedSignature OnMarketOpened;

	UPROPERTY(BlueprintAssignable, Category = "PD|Market|Events")
	FPDMarketStateChangedSignature OnMarketClosed;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Market")
	TObjectPtr<UPDMarketComponent> MarketComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Sound")
	TObjectPtr<USoundBase> MarketOpenSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Market|Sound")
	TObjectPtr<USoundBase> MarketCloseSound;
private:
	virtual bool IsStationOpen(APDPlayerController* PlayerController) const override;
	virtual void OpenStation(APDPlayerController* PlayerController) override;
	virtual void CloseStation(APDPlayerController* PlayerController) override;
	virtual void BindStationClose(APDPlayerController* PlayerController) override;
	virtual void UnbindStationClose() override;
	virtual void HandleStationOpened(APDPlayerController* PlayerController) override;
	void HandleMarketInterfaceClosed(UPDMarketComponent* ClosedMarketComponent);
};
