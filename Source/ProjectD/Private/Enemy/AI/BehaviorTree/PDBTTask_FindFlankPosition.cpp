#include "Enemy/AI/BehaviorTree/PDBTTask_FindFlankPosition.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"

UPDBTTask_FindFlankPosition::UPDBTTask_FindFlankPosition()
{
	NodeName = TEXT("PD Find Flank Position");

	BlackboardKey.SelectedKeyName = PDBTKeys::TargetActor;

	OutputLocationKey.SelectedKeyName = PDBTKeys::FlankLocation;
}

void UPDBTTask_FindFlankPosition::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	// 출력 키는 Vector 만 허용.
	OutputLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UPDBTTask_FindFlankPosition, OutputLocationKey));

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		OutputLocationKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UPDBTTask_FindFlankPosition::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject(BlackboardKey.SelectedKeyName)) : nullptr;

	if (!Pawn || !Target || !BB) return EBTNodeResult::Failed;

	UWorld* World = GetWorld();
	UNavigationSystemV1* NavSys = World ? UNavigationSystemV1::GetCurrent(World) : nullptr;
	if (!World || !NavSys) return EBTNodeResult::Failed;

	const FVector PawnLoc = Pawn->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();
	const FVector ToTargetDir2D = (TargetLoc - PawnLoc).GetSafeNormal2D();
	if (ToTargetDir2D.IsNearlyZero()) return EBTNodeResult::Failed;

	const float MinAng = FMath::Min(MinFlankAngleDegrees, MaxFlankAngleDegrees);
	const float MaxAng = FMath::Max(MinFlankAngleDegrees, MaxFlankAngleDegrees);
	const FVector ProjectExtent(NavProjectExtent, NavProjectExtent, NavProjectExtent * 0.5f);

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(PD_BT_FlankLOS), /*bTraceComplex=*/true, Pawn);
	TraceParams.AddIgnoredActor(Target);

	// 좌/우 번갈아가며 평가 — 좁은 각도부터 시작해서 점차 측면/후방으로.
	// 첫 LOS 확보 후보를 즉시 채택해서 BT 분기 지연 최소화.
	for (int32 i = 0; i < NumSamplesPerSide; ++i)
	{
		const float Alpha = (NumSamplesPerSide == 1) ? 0.f : static_cast<float>(i) / (NumSamplesPerSide - 1);
		const float Angle = FMath::Lerp(MinAng, MaxAng, Alpha);

		for (int32 SideSign : { -1, +1 })
		{
			const FRotator OffsetRot(0.f, SideSign * Angle, 0.f);
			const FVector Dir = OffsetRot.RotateVector(ToTargetDir2D);
			const FVector RawCandidate = PawnLoc + Dir * FlankRadius;

			FNavLocation NavLoc;
			if (!NavSys->ProjectPointToNavigation(RawCandidate, NavLoc, ProjectExtent)) continue;

			const FVector TraceStart = NavLoc.Location + FVector(0.f, 0.f, TraceEyeHeight);
			FHitResult Hit;
			const bool bBlocked = World->LineTraceSingleByChannel(
				Hit, TraceStart, TargetLoc, ECC_Visibility, TraceParams);

			if (bBlocked) continue;

			BB->SetValueAsVector(OutputLocationKey.SelectedKeyName, NavLoc.Location);
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
