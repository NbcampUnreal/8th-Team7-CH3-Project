#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PDInteractable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPDInteractable : public UInterface
{
	GENERATED_BODY()
};

class PROJECTD_API IPDInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "PD|Interact")
	void Interact(AActor* Interactor);

	// 현재 이 대상이 상호작용(프롬프트 표시) 가능한지. false면 탐지/프롬프트에서 제외.
	// 기본 true(루트박스/스테이션 등). 캐릭터는 오버라이드해 적=false, 플레이어=Downed 일 때만 true.
	UFUNCTION(BlueprintNativeEvent, Category = "PD|Interact")
	bool CanInteract(AActor* Interactor) const;
	virtual bool CanInteract_Implementation(AActor* Interactor) const { return true; }

	UFUNCTION(BlueprintNativeEvent, Category = "PD|Interact")
	FText GetInteractDisplayText() const;

	// override 안 한 클래스는 빈 텍스트 — 위젯엔 키 아이콘만 표시되고 액션 라벨 자리는 공백.
	virtual FText GetInteractDisplayText_Implementation() const { return FText::GetEmpty(); }

	// 액터 메시별 위치 미세조정용. BP에서 override해 액터 머리/문/창고 윗부분 등으로 세팅.

	UFUNCTION(BlueprintNativeEvent, Category = "PD|Interact")
	FVector GetInteractPromptOffset() const;

	// override 안 한 클래스는 액터 origin 사용 (오프셋 0).
	virtual FVector GetInteractPromptOffset_Implementation() const { return FVector::ZeroVector; }
};
