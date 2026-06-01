


#include "Component/PDWeaponComponent.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "Engine/EngineTypes.h"

UPDWeaponComponent::UPDWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPDWeaponComponent::SetAimTarget(AActor* Target)
{
    AimTarget = Target;
}

void UPDWeaponComponent::ClearAimTarget()
{
    AimTarget = nullptr;
}

FVector UPDWeaponComponent::GetAimDirection(const FVector& StartLocation) const
{
    AActor* Owner = GetOwner();
    if (!Owner) return FVector::ForwardVector;


    if (AimTarget.IsValid())
    {
        FVector TargetLoc = AimTarget->GetActorLocation() + FVector(0.f, 0.f, 80.f);
        FVector Dir = TargetLoc - StartLocation;
        if (!Dir.IsNearlyZero()) return Dir.GetSafeNormal();
    }


    APlayerController* PC = Cast<APlayerController>(Owner->GetInstigatorController());
    if (PC)
    {
        if (const APDPlayerController* PDPC = Cast<APDPlayerController>(PC))
        {
            FVector CachedAimLocation;
            if (PDPC->GetCachedAimWorldLocation(CachedAimLocation))
            {
                FVector Dir = CachedAimLocation - StartLocation;
                if (!Dir.IsNearlyZero()) return Dir.GetSafeNormal();
            }
        }


        FHitResult PawnHit;
        const TArray<TEnumAsByte<EObjectTypeQuery>> PawnObjectTypes = { UEngineTypes::ConvertToObjectType(ECC_Pawn) };
        const bool bPawnHit = Cast<APDPlayerController>(PC)
            ? Cast<APDPlayerController>(PC)->GetRecoiledHitResultForObjects(PawnObjectTypes, true, PawnHit)
            : PC->GetHitResultUnderCursorForObjects(PawnObjectTypes, true, PawnHit);
        if (bPawnHit
            && PawnHit.GetActor() && PawnHit.GetActor() != Owner)
        {
            FVector Dir = PawnHit.Location - StartLocation;
            if (!Dir.IsNearlyZero()) return Dir.GetSafeNormal();
        }

        FHitResult CursorHit;
        const bool bCursorHit = Cast<APDPlayerController>(PC)
            ? Cast<APDPlayerController>(PC)->GetRecoiledHitResult(ECC_Visibility, true, CursorHit)
            : PC->GetHitResultUnderCursor(ECC_Visibility, true, CursorHit);
        if (bCursorHit)
        {
            FVector Dir = CursorHit.Location - StartLocation;
            if (!Dir.IsNearlyZero()) return Dir.GetSafeNormal();
        }
    }


    return Owner->GetActorForwardVector();
}
