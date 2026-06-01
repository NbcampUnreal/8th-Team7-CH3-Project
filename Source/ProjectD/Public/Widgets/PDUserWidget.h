#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDUserWidget.generated.h"

class UButton;
class USoundBase;

UCLASS(Abstract, BlueprintType)
class PROJECTD_API UPDUserWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleButtonClickedSound();

	void BindButtonClickSounds();
	void UnbindButtonClickSounds();
	USoundBase* ResolveButtonClickSound() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> BoundSoundButtons;
};
