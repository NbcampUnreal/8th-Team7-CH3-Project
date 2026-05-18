#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDFaintMarkActor.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDFaintMarkActor : public AActor
{
	GENERATED_BODY()

public:
	APDFaintMarkActor();

	//Subsystem이 스폰 직후 호출
	UFUNCTION(BlueprintCallable, Category="FaintMark")
	void InitializeFaintMark(int32 InFaintId);

	UFUNCTION(BlueprintPure, Category="FaintMark")
	FORCEINLINE int32 GetFaintId() const { return FaintId; }

protected:
	//BP에서 구현
	UFUNCTION(BlueprintImplementableEvent, Category="FaintMark")
	void OnFaintMarkInitialized(int32 InFaintId);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

private:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="FaintMark", meta=(AllowPrivateAccess="true"))
	int32 FaintId = -1;
};