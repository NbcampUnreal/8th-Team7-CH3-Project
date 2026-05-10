#pragma once

#include "CoreMinimal.h"
#include "Widgets/PDActivatableBase.h"
#include "PDMinimapWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTD_API UPDMinimapWidget : public UPDActivatableBase
{
	GENERATED_BODY()

public:
	//미니맵 표시 반경 (월드 단위, cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minimap")
	float MapRadius = 2000.f;

protected:
	virtual void NativeTick(const FGeometry& Geo, float DeltaTime) override;

	//BP에서 갱신 (플레이어 위치/회전)
	UFUNCTION(BlueprintImplementableEvent, Category="Minimap")
	void OnPlayerTransformUpdated(const FVector& Location, float Yaw);
};