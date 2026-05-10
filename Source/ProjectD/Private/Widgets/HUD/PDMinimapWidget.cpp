#include "Widgets/HUD/PDMinimapWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

void UPDMinimapWidget::NativeTick(const FGeometry& Geo, float DeltaTime)
{
	Super::NativeTick(Geo, DeltaTime);

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			const FVector Loc = Pawn->GetActorLocation();
			const float Yaw = Pawn->GetActorRotation().Yaw;
			OnPlayerTransformUpdated(Loc, Yaw);
		}
	}
}