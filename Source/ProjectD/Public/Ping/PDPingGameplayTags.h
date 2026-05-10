#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * 핑 시스템 전용 GameplayTag.
 * (PDGameplayTags / PDFrontendGameplayTags 와 분리해서 모듈별 응집도 유지)
 */
namespace PDPingGameplayTags
{
	// 핑 입력. PDInputConfig 데이터 에셋에서 IA_Ping 과 매핑.
	PROJECTD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ping);
}