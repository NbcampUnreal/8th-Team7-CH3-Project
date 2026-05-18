#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDMapMarkerActor.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROJECTD_API APDMapMarkerActor : public AActor
{
	GENERATED_BODY()

public:
	APDMapMarkerActor();

	//Subsystem이 스폰 직후 호출
	UFUNCTION(BlueprintCallable, Category="MapMarker")
	void InitializeMarker(int32 InMarkerId, int32 InDisplayIndex);

	//마커 재정렬 시 호출(번호 변경)
	UFUNCTION(BlueprintCallable, Category="MapMarker")
	void UpdateDisplayIndex(int32 InDisplayIndex);

	UFUNCTION(BlueprintPure, Category="MapMarker")
	FORCEINLINE int32 GetMarkerId() const {return MarkerId;}

	UFUNCTION(BlueprintPure, Category="MapMarker")
	FORCEINLINE int32 GetDisplayIndex() const {return DisplayIndex;}

protected:
	//BP에서 구현(메시 표시, 번호 텍스트 위젯 등)
	UFUNCTION(BlueprintImplementableEvent, Category="MapMarker")
	void OnMarkerInitialized(int32 InMarkerId, int32 InDisplayIndex);

	//BP에서 구현(번호만 갱신)
	UFUNCTION(BlueprintImplementableEvent, Category="MapMarker")
	void OnDisplayIndexUpdated(int32 InDisplayIndex);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

private:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="MapMarker", meta=(AllowPrivateAccess="true"))
	int32 MarkerId = -1;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="MapMarker", meta=(AllowPrivateAccess="true"))
	int32 DisplayIndex = 0;
};