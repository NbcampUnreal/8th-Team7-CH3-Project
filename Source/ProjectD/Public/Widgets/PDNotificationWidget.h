#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDNotificationWidget.generated.h"

class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDNotificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Notification")
	void ShowNotification(const FText& Message, float Duration = 2.f);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "PD|Notification")
	TObjectPtr<UTextBlock> Text_Message;

	virtual void NativeDestruct() override;

private:
	FTimerHandle HideTimerHandle;

	void HideNotification();
};
