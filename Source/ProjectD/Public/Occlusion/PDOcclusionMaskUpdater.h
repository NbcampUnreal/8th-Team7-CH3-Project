#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDOcclusionMaskUpdater.generated.h"

class UMaterialParameterCollection;

//소유 액터(캐릭터)의 월드 위치를 매 틱 MPC_PDOcclusion 의 PlayerLocation 에 갱신
//MF_PDOcclusionMask 가 캐릭터를 중심으로 캡슐 마스크를 따라가게 함
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDOcclusionMaskUpdater : public UActorComponent
{
    GENERATED_BODY()

public:
    UPDOcclusionMaskUpdater();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
    //갱신할 MPC (= MPC_PDOcclusion 지정)
    UPROPERTY(EditAnywhere, Category="Occlusion")
    TObjectPtr<UMaterialParameterCollection> ParameterCollection;

    //갱신할 Vector 파라미터 이름 (기본: PlayerLocation)
    UPROPERTY(EditAnywhere, Category="Occlusion")
    FName PlayerLocationParameterName = TEXT("PlayerLocation");
};