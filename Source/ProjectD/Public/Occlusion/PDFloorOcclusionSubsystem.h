#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PDFloorOcclusionSubsystem.generated.h"

class UPDFloorOcclusionComponent;

UCLASS()
class PROJECTD_API UPDFloorOcclusionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	void RegisterFloorActor(UPDFloorOcclusionComponent* Component);
	void UnregisterFloorActor(UPDFloorOcclusionComponent* Component);

	//특정 빌딩 내에서 VisibleFloor 이상만 페이드 아웃.
	void SetVisibleBuildingFloor(FName BuildingGroupID, int32 VisibleFloor);

	//빌딩 이탈 시 그 빌딩 전체 페이드 인.
	void ClearVisibleBuilding(FName BuildingGroupID);

private:
	//빌딩별 + 층별 컴포넌트
	TMap<FName, TMap<int32, TArray<TWeakObjectPtr<UPDFloorOcclusionComponent>>>> ComponentsByBuildingFloor;

	//빌딩별 현재 가시 층 INDEX_NONE = 외부 (모두 보임)
	TMap<FName, int32> VisibleFloorByBuilding;

	struct FFadeTarget
	{
		TWeakObjectPtr<UPDFloorOcclusionComponent> Component;
		float TargetAlpha = 1.0f;
	};
	TArray<FFadeTarget> ActiveFades;

	//초당 alpha 변화량 4.0이면 0.25초에 0→1
	float FadeSpeed = 1.0f;

	void RequestFade(UPDFloorOcclusionComponent* Component, float TargetAlpha);
	float CalculateTargetAlpha(FName BuildingGroupID, int32 FloorLevel) const;
	void RefreshBuildingFades(FName BuildingGroupID);
};