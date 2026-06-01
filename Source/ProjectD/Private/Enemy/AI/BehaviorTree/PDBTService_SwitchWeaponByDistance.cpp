#include "Enemy/AI/BehaviorTree/PDBTService_SwitchWeaponByDistance.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/BehaviorTree/PDBTKeys.h"
#include "Enemy/Characters/PDEliteSoldier.h"
#include "GameFramework/Pawn.h"

UPDBTService_SwitchWeaponByDistance::UPDBTService_SwitchWeaponByDistance()
{
	NodeName = TEXT("PD Switch Weapon By Distance");
	bNotifyTick = true;
	// 무기 swap 은 spawn/destroy 비용이 있어 너무 자주 돌릴 필요 없음.
	Interval = 0.5f;
	RandomDeviation = 0.1f;
}

void UPDBTService_SwitchWeaponByDistance::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AI = OwnerComp.GetAIOwner();
	APDEliteSoldier* Elite = AI ? Cast<APDEliteSoldier>(AI->GetPawn()) : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Elite || !BB) return;

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(PDBTKeys::TargetActor));
	if (!IsValid(Target)) return;

	const float Dist = FVector::Dist(Elite->GetActorLocation(), Target->GetActorLocation());
	Elite->SwitchWeaponByDistance(Dist);
}
