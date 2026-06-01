#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "PDEnvQueryContext_CombatTarget.generated.h"

/**
 * EQS 컨텍스트 — Querier 의 UPDCombatComponent.GetCurrentTarget() 반환.
 * 디자이너가 Trace/Distance/Dot 테스트의 Context 칸에 지정 → 타겟 기준 점수 산출.
 * Querier 에 UPDCombatComponent 가 없으면 빈 컨텍스트.
 */
UCLASS()
class PROJECTD_API UPDEnvQueryContext_CombatTarget : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
