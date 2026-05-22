#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/Screen/PDTabbedContent.h"
#include "PDPlayerSkillWindowWidget.generated.h"

/**
 * Player Hub의 Skill 탭 placeholder. 실제 스킬 시스템 구현 전까지 "준비 중" 표시용.
 * DA에서 bEnabled=false로 두면 탭 자체가 비활성 표시된다.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class PROJECTD_API UPDPlayerSkillWindowWidget : public UUserWidget, public IPDTabbedContent
{
	GENERATED_BODY()

public:
	virtual void InitializeForOwner(APlayerController* OwnerPC) override;
	virtual void OnTabShown() override;
	virtual void OnTabHidden() override;
};