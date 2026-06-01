#include "Enemy/AI/EQS/PDEnvQueryContext_CombatTarget.h"

#include "AIController.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "Enemy/Components/PDCombatComponent.h"

void UPDEnvQueryContext_CombatTarget::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	// QueryInstance.Owner 는 EQS 실행 시 SetOwner 한 액터(보통 AIController 또는 Pawn).
	AActor* Querier = Cast<AActor>(QueryInstance.Owner.Get());
	if (!Querier) return;

	// AIController 일 경우 컴포넌트 검색은 Pawn 측.
	if (AAIController* AI = Cast<AAIController>(Querier))
	{
		Querier = AI->GetPawn();
	}
	if (!Querier) return;

	UPDCombatComponent* Combat = Querier->FindComponentByClass<UPDCombatComponent>();
	if (!Combat) return;

	AActor* Target = Combat->GetCurrentTarget();
	if (!Target) return;

	UEnvQueryItemType_Actor::SetContextHelper(ContextData, Target);
}
