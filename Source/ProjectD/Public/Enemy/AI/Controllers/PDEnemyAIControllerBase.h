#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "PDEnemyAIControllerBase.generated.h"

class UBehaviorTree;
class UBehaviorTreeComponent;
class UBlackboardComponent;
class UPDPerceptionComponent;
struct FAIStimulus;

/** AI 진입 체인(Possess → Perception → BT Task → MoveTo) 디버그 단일 채널. */
PROJECTD_API DECLARE_LOG_CATEGORY_EXTERN(LogPDAI, Log, All);

/**
 * Enemy 전용 AIController 베이스 (BT/BB 기반).
 *
 * 책임:
 *  - UBehaviorTreeComponent + UBlackboardComponent 보유 및 RunBehaviorTree.
 *  - UPDPerceptionComponent (시각/청각) 주입 — 부모 AAIController.PerceptionComponent 슬롯 사용.
 *  - Perception 이벤트 → CombatComponent + Blackboard 키 동기화.
 *  - Pawn의 EnemyState 변화를 BB에 반영.
 *
 * 디자이너 작업 흐름:
 *  - BP_PDEnemyAIControllerBase 자식 클래스에서 BehaviorTreeAsset / BlackboardAsset 지정.
 *  - BB 키 이름은 PDBTKeys.h 의 상수와 일치시킬 것.
 *
 * 결합 분리: 본 클래스는 "AI 인프라 조립"만 담당 — 의사결정은 BT에, 액션은 컴포넌트에 위임.
 */
UCLASS()
class PROJECTD_API APDEnemyAIControllerBase : public AAIController
{
	GENERATED_BODY()

public:
	APDEnemyAIControllerBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaTime) override;

	// 친화도 결정 단일 진실 원천 — 엔진 디폴트의 타이밍/솔버 모호함 회피.
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	UFUNCTION(BlueprintPure, Category = "PD|AI")
	UPDPerceptionComponent* GetPDPerception() const;

	UFUNCTION(BlueprintPure, Category = "PD|AI")
	FORCEINLINE UBehaviorTreeComponent* GetBehaviorTreeComponent() const { return BehaviorTreeComponent; }

	UFUNCTION(BlueprintPure, Category = "PD|AI")
	UBlackboardComponent* GetBlackboard() const { return const_cast<UBlackboardComponent*>(GetBlackboardComponent()); }

	/** 폰 사망 시 호출 — BT 즉시 정지(Safe). 컨트롤러/구독은 보존, 최종 정리는 OnUnPossess(액터 소멸 시)에 위임. */
	UFUNCTION(BlueprintCallable, Category = "PD|AI")
	void NotifyPawnDied();

protected:
	/** 디자이너가 BP 디폴트에서 지정. nullptr 이면 BT 미실행 — 경고 로그 후 통과. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent;

	// 주의: PerceptionComponent / BlackboardComponent 는 부모(AAIController)의 UPROPERTY 슬롯을 사용.
	//      자식에서 같은 이름으로 다시 선언하면 UHT shadowing 오류.

private:
	UFUNCTION() void HandleTargetSpotted(AActor* Target);
	UFUNCTION() void HandleTargetLost   (AActor* Target, FVector LastKnownLocation);
	UFUNCTION() void HandleNoiseHeard   (AActor* NoiseInstigator, FVector Location);
	UFUNCTION() void HandleCombatTargetChanged(AActor* NewTarget);

	void StartBehaviorTree();
	void DrawAIDebug() const;

protected:
	/** BP 확장 hook — BP 측에서 추가 반응 작성 가능. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|AI")
	void OnTargetSpotted(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|AI")
	void OnTargetLost(AActor* Target, FVector LastKnownLocation);

	UFUNCTION(BlueprintImplementableEvent, Category = "PD|AI")
	void OnNoiseHeard(AActor* NoiseInstigator, FVector Location);
};
