#include "Enemy/AI/BehaviorTree/PDBTTask_RotateToFaceTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "GameFramework/Pawn.h"

namespace
{
	struct FPDRotateMemory
	{
		float ElapsedTime = 0.f;
	};
}

UPDBTTask_RotateToFaceTarget::UPDBTTask_RotateToFaceTarget()
{
	NodeName = TEXT("PD Rotate To Face BB");
	bNotifyTick = true;

	// Object(AActor*) 또는 Vector 키 허용. 디자이너가 BB 자산에서 두 타입 중 하나 선택.
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UPDBTTask_RotateToFaceTarget, BlackboardKey), AActor::StaticClass());
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UPDBTTask_RotateToFaceTarget, BlackboardKey));

	BlackboardKey.SelectedKeyName = PDBTKeys::TargetActor;
}

uint16 UPDBTTask_RotateToFaceTarget::GetInstanceMemorySize() const
{
	return sizeof(FPDRotateMemory);
}

bool UPDBTTask_RotateToFaceTarget::ResolveTargetLocation(UBehaviorTreeComponent& OwnerComp, FVector& OutLocation) const
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return false;

	const FName KeyName = BlackboardKey.SelectedKeyName;
	const auto KeyType = BlackboardKey.SelectedKeyType;

	if (KeyType == UBlackboardKeyType_Object::StaticClass())
	{
		if (AActor* Actor = Cast<AActor>(BB->GetValueAsObject(KeyName)))
		{
			OutLocation = Actor->GetActorLocation();
			return true;
		}
		return false;
	}
	if (KeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		const FVector V = BB->GetValueAsVector(KeyName);
		if (V.ContainsNaN()) return false;
		OutLocation = V;
		return true;
	}
	return false;
}

EBTNodeResult::Type UPDBTTask_RotateToFaceTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!Pawn) return EBTNodeResult::Failed;

	FVector TargetLocation;
	if (!ResolveTargetLocation(OwnerComp, TargetLocation)) return EBTNodeResult::Failed;

	FPDRotateMemory* Memory = new (NodeMemory) FPDRotateMemory();
	Memory->ElapsedTime = 0.f;

	// 이미 허용 오차 안이면 Tick 없이 즉시 완료.
	const FVector ToTarget = (TargetLocation - Pawn->GetActorLocation()).GetSafeNormal2D();
	if (!ToTarget.IsNearlyZero())
	{
		const float DesiredYaw = ToTarget.Rotation().Yaw;
		const float CurrentYaw = Pawn->GetActorRotation().Yaw;
		if (FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentYaw, DesiredYaw)) <= AngleToleranceDegrees)
		{
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::InProgress;
}

void UPDBTTask_RotateToFaceTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!Pawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FVector TargetLocation;
	if (!ResolveTargetLocation(OwnerComp, TargetLocation))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FPDRotateMemory* Memory = reinterpret_cast<FPDRotateMemory*>(NodeMemory);
	Memory->ElapsedTime += DeltaSeconds;

	const FVector ToTarget = (TargetLocation - Pawn->GetActorLocation()).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	const FRotator CurrentRot = Pawn->GetActorRotation();
	const FRotator DesiredRot(0.f, ToTarget.Rotation().Yaw, 0.f);

	const float DeltaYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRot.Yaw, DesiredRot.Yaw));
	if (DeltaYaw <= AngleToleranceDegrees)
	{
		Pawn->SetActorRotation(FRotator(CurrentRot.Pitch, DesiredRot.Yaw, CurrentRot.Roll));
		AIController->SetControlRotation(FRotator(0.f, DesiredRot.Yaw, 0.f));
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	if (TimeoutSeconds > 0.f && Memory->ElapsedTime >= TimeoutSeconds)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// Pitch/Roll 보존, Yaw 만 보간.
	const FRotator TargetYawOnly(CurrentRot.Pitch, DesiredRot.Yaw, CurrentRot.Roll);
	const FRotator NewRot = FMath::RInterpConstantTo(CurrentRot, TargetYawOnly, DeltaSeconds, RotationSpeedDegPerSec);
	Pawn->SetActorRotation(NewRot);
	AIController->SetControlRotation(FRotator(0.f, NewRot.Yaw, 0.f));
}
