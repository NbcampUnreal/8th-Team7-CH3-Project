#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PDFaintMarkWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTD_API UPDFaintMarkWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	//Subsystem의 고유 식별자
	UPROPERTY(BlueprintReadWrite, Category="Ping")
	int32 FaintId = -1;

	//잔존 표식 월드 좌표
	UPROPERTY(BlueprintReadWrite, Category="Ping")
	FVector WorldLocation = FVector::ZeroVector;

protected:
	//우클릭 시 삭제 요청
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
};