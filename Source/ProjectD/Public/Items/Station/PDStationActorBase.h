#pragma once

#include "CoreMinimal.h"
#include "Items/Station/PDInteractableActorBase.h"
#include "PDStationActorBase.generated.h"

class APDPlayerController;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API APDStationActorBase : public APDInteractableActorBase
{
	GENERATED_BODY()

public:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Interact_Implementation(AActor* Interactor) override;

protected:
	virtual bool IsStationOpen(APDPlayerController* PlayerController) const;
	virtual void OpenStation(APDPlayerController* PlayerController);
	virtual void CloseStation(APDPlayerController* PlayerController);
	virtual bool DidStationOpen(APDPlayerController* PlayerController) const;
	virtual void BindStationClose(APDPlayerController* PlayerController);
	virtual void UnbindStationClose();
	virtual void HandleStationOpened(APDPlayerController* PlayerController);
	virtual void HandleStationClosed();

	TWeakObjectPtr<APDPlayerController> BoundPlayerController;
};
