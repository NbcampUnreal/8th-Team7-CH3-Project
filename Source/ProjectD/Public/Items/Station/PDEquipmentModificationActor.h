#pragma once

#include "CoreMinimal.h"
#include "Items/Station/PDStationActorBase.h"
#include "PDEquipmentModificationActor.generated.h"

class APDPlayerController;
class UStaticMeshComponent;

class APDEquipmentModificationActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDEquipmentModificationStateChangedSignature, APDEquipmentModificationActor*, ModificationActor);

UCLASS(Blueprintable)
class PROJECTD_API APDEquipmentModificationActor : public APDStationActorBase
{
	GENERATED_BODY()

public:
	APDEquipmentModificationActor();

	UPROPERTY(BlueprintAssignable, Category = "PD|Equipment Modification|Events")
	FPDEquipmentModificationStateChangedSignature OnEquipmentModificationOpened;

	UPROPERTY(BlueprintAssignable, Category = "PD|Equipment Modification|Events")
	FPDEquipmentModificationStateChangedSignature OnEquipmentModificationClosed;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Equipment Modification")
	TObjectPtr<UStaticMeshComponent> WorkbenchMesh;

private:
	virtual bool IsStationOpen(APDPlayerController* PlayerController) const override;
	virtual void OpenStation(APDPlayerController* PlayerController) override;
	virtual void CloseStation(APDPlayerController* PlayerController) override;
	virtual void BindStationClose(APDPlayerController* PlayerController) override;
	virtual void UnbindStationClose() override;
	virtual void HandleStationOpened(APDPlayerController* PlayerController) override;
	void HandleEquipmentModificationInterfaceClosed();
};
