#include "Enemy/AI/BehaviorTree/PDBTTask_ClearBBValue.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

UPDBTTask_ClearBBValue::UPDBTTask_ClearBBValue()
{
	NodeName = TEXT("PD Clear BB Value");

	// UE5.7: bAllowAllTypes 가 제거됨 — 허용 타입을 명시 등록해야 함.
	const FName KeyPropName = GET_MEMBER_NAME_CHECKED(UPDBTTask_ClearBBValue, TargetKey);
	TargetKey.AddObjectFilter (this, KeyPropName, UObject::StaticClass());
	TargetKey.AddClassFilter  (this, KeyPropName, UObject::StaticClass());
	TargetKey.AddVectorFilter (this, KeyPropName);
	TargetKey.AddRotatorFilter(this, KeyPropName);
	TargetKey.AddBoolFilter   (this, KeyPropName);
	TargetKey.AddIntFilter    (this, KeyPropName);
	TargetKey.AddFloatFilter  (this, KeyPropName);
	TargetKey.AddStringFilter (this, KeyPropName);
	TargetKey.AddNameFilter   (this, KeyPropName);
}

void UPDBTTask_ClearBBValue::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UPDBTTask_ClearBBValue::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;
	if (TargetKey.SelectedKeyName.IsNone()) return EBTNodeResult::Failed;

	BB->ClearValue(TargetKey.SelectedKeyName);
	return EBTNodeResult::Succeeded;
}

FString UPDBTTask_ClearBBValue::GetStaticDescription() const
{
	return FString::Printf(TEXT("Clear: %s"), *TargetKey.SelectedKeyName.ToString());
}
