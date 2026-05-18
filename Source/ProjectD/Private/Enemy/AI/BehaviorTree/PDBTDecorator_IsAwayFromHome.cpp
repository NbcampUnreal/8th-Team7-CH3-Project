#include "Enemy/AI/BehaviorTree/PDBTDecorator_IsAwayFromHome.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "GameFramework/Pawn.h"

UPDBTDecorator_IsAwayFromHome::UPDBTDecorator_IsAwayFromHome()
{
	NodeName = TEXT("PD Is Away From Home");

	// 기본 입력: HomeLocation (Vector). Vector 키만 허용.
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UPDBTDecorator_IsAwayFromHome, BlackboardKey));
	BlackboardKey.SelectedKeyName = PDBTKeys::HomeLocation;
}

bool UPDBTDecorator_IsAwayFromHome::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/) const
{
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return false;

	const AAIController* AICon = OwnerComp.GetAIOwner();
	const APawn* Pawn = AICon ? AICon->GetPawn() : nullptr;
	if (!Pawn) return false;

	const FVector HomeLoc = BB->GetValueAsVector(BlackboardKey.SelectedKeyName);
	// Invalid Location (FLT_MAX) 가드 — OnPossess 전에 BB 가 비어있는 케이스.
	if (HomeLoc.ContainsNaN() || !FMath::IsFinite(HomeLoc.X)) return false;

	const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), HomeLoc);
	return DistSq > (AwayDistance * AwayDistance);
}

FString UPDBTDecorator_IsAwayFromHome::GetStaticDescription() const
{
	return FString::Printf(TEXT("Pawn-Home dist > %.0f"), AwayDistance);
}
