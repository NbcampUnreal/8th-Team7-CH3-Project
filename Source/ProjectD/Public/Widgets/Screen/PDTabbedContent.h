#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PDTabbedContent.generated.h"

class APlayerController;
class UWidget;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UPDTabbedContent : public UInterface
{
	GENERATED_BODY()
};

/**
 * 탭 컨테이너(UPDTabbedScreenBase) 안에 임베드되는 컨텐츠 위젯 컨트랙트.
 * base는 컨텐츠 클래스를 직접 Cast하지 않고 이 인터페이스로만 통신한다.
 */
class PROJECTD_API IPDTabbedContent
{
	GENERATED_BODY()

public:
	/** 컨텐츠 스폰 직후 한 번 호출. PC로부터 필요한 컴포넌트를 디스커버. */
	virtual void InitializeForOwner(APlayerController* OwnerPC) {}

	/** 탭이 활성화될 때. */
	virtual void OnTabShown() {}

	/** 탭이 가려질 때(다른 탭으로 전환되거나 screen이 닫힐 때). */
	virtual void OnTabHidden() {}

	/** 활성 시 포커스를 줄 대상. nullptr 가능. */
	virtual UWidget* GetTabDesiredFocusTarget() const { return nullptr; }
};