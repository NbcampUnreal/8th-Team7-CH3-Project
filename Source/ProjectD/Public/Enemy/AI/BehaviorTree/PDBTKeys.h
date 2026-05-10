#pragma once

#include "CoreMinimal.h"

/**
 * Blackboard 키 이름 단일 진실 원천.
 * BT/Decorator/Service/Task 와 BB 자산이 같은 문자열을 공유하도록 강제하기 위함.
 *  - 디자이너는 BB 자산에서 동일한 이름의 키를 만들어야 함.
 *  - C++ 측은 본 헤더의 상수만 사용.
 */
namespace PDBTKeys
{
	// ─── Perception / Combat ───
	static const FName TargetActor       = TEXT("TargetActor");        // Object  (감지된 시각 타겟)
	static const FName HasNoiseHint      = TEXT("HasNoiseHint");       // Bool
	static const FName LastNoiseLocation = TEXT("LastNoiseLocation");  // Vector
	static const FName LastSeenLocation  = TEXT("LastSeenLocation");   // Vector  (Chase 용)

	// ─── State / Movement ───
	static const FName EnemyState        = TEXT("EnemyState");         // Enum    EPDEnemyState
	static const FName HomeLocation      = TEXT("HomeLocation");       // Vector  (스폰 지점, Patrol 기준)
	static const FName PatrolRadius      = TEXT("PatrolRadius");       // Float
	static const FName AttackRange       = TEXT("AttackRange");        // Float

	// ─── Combat ability cache ───
	static const FName CanAttack         = TEXT("CanAttack");          // Bool    (Service 가 갱신)
	static const FName IsTargetInRange   = TEXT("IsTargetInRange");    // Bool    (Service 가 갱신 — 거리 변화 옵저버 트리거용)
	static const FName HasLOSToTarget    = TEXT("HasLOSToTarget");     // Bool    (Service 가 갱신 — 시야 변화 옵저버 트리거용)
}
