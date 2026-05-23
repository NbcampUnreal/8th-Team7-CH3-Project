#include "Occlusion/PDOcclusionMaskUpdater.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMaterialLibrary.h"

UPDOcclusionMaskUpdater::UPDOcclusionMaskUpdater()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UPDOcclusionMaskUpdater::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!ParameterCollection)
    {
        return;
    }

    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    //월드 좌표를 LinearColor 로 패킹 (R=X, G=Y, B=Z) - 함수의 ComponentMask 가 R/G/B 만 사용
    const FVector OwnerLocation = Owner->GetActorLocation();
    const FLinearColor LocationAsColor(OwnerLocation.X, OwnerLocation.Y, OwnerLocation.Z, 0.0f);

    UKismetMaterialLibrary::SetVectorParameterValue(
        this,
        ParameterCollection,
        PlayerLocationParameterName,
        LocationAsColor
    );
}