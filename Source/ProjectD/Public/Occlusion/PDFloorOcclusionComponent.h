#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDFloorOcclusionComponent.generated.h"

class UMaterialInstanceDynamic;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDFloorOcclusionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDFloorOcclusionComponent();

	//볼륨이 동적 부착할 때 호출하는 초기화
	void InitializeFloorInfo(FName InBuildingGroupID, int32 InFloorLevel);

	UFUNCTION(BlueprintPure, Category = "Floor")
	FName GetBuildingGroupID() const { return BuildingGroupID; }

	UFUNCTION(BlueprintPure, Category = "Floor")
	int32 GetFloorLevel() const { return FloorLevel; }
	
	//2차 디더 폴리싱 시 이 함수 내부만 머티리얼 파라미터 보간으로 교체.
	void SetFadeAmount(float Alpha);

	float GetFadeAmount() const { return CurrentFadeAmount; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor")
	FName BuildingGroupID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor")
	int32 FloorLevel = 0;

private:
	float CurrentFadeAmount = 1.0f;
	bool bRegistered = false;
	
	//SetFadeAmount호출 시 DitherFade Scalar Parameter 갱신 대상
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DitherMaterials;
};