#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/Screen/PDTabbedContent.h"
#include "PDPlayerHealthDetailWidget.generated.h"

/**
 * Player Hub의 Health 탭 placeholder.
 * 추후 paperdoll(LimbHP_*) 통합 지점. 현 단계는 인터페이스 마운트와 BP 디자인용 스텁.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDPlayerHealthDetailWidget : public UUserWidget, public IPDTabbedContent
{
	GENERATED_BODY()

public:
	virtual void InitializeForOwner(APlayerController* OwnerPC) override;
	virtual void OnTabShown() override;
	virtual void OnTabHidden() override;
};