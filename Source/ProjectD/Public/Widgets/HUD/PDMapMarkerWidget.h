#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDMapMarkerWidget.generated.h"

class UTextBlock;

UCLASS(Abstract, Blueprintable)
class PROJECTD_API UPDMapMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	//WorldMap이 위젯 추적
	UPROPERTY(BlueprintReadWrite, Category="Map")
	int32 MarkerId = -1;

	//마커 월드 좌표
	UPROPERTY(BlueprintReadWrite, Category="Map")
	FVector WorldLocation = FVector::ZeroVector;

	//표시 번호 외부에서 갱신용
	UFUNCTION(BlueprintCallable, Category="Map")
	void SetDisplayIndex(int32 InIndex);

protected:
	//번호 표시 텍스트
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> NumberText;

	//우클릭 받으면 자기 자신 삭제 요청//
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
};