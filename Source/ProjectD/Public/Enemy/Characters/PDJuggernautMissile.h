#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PDJuggernautMissile.generated.h"

class UStaticMeshComponent;
class UParticleSystem;
class USoundBase;

/**
 * Juggernaut 패턴2(섬멸) 미사일 — 코스메틱.
 *  - 해치에서 지정 착탄지점으로 포물선 비행 → 도착 시 폭발 연출 후 자가 소멸.
 *  - 데미지는 보스(서버)가 착탄 시점에 AoE 로 처리. 본 액터는 비주얼 전용이라 머신별 로컬 스폰(비복제).
 */
UCLASS()
class PROJECTD_API APDJuggernautMissile : public AActor
{
	GENERATED_BODY()

public:
	APDJuggernautMissile();

	/** 비행 설정: 시작(해치)·착탄지점·비행시간·착탄반경(인디케이터 스케일용). 스폰 직후 호출. */
	void InitMissile(const FVector& InStart, const FVector& InImpact, float InTravelTime, float InImpactRadius);

protected:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Missile", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Missile", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MissileMesh;

	/** 착탄지점 바닥 인디케이터(절대 위치 — 미사일이 날아가도 고정). 평면 메시 + M_PD_AoECircle 지정. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Missile", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ImpactIndicator;

	/** 인디케이터 평면 메시 기본 한 변 길이(uu). 엔진 Plane=100. 착탄반경에 맞춰 자동 스케일. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Missile", meta = (ClampMin = "1.0"))
	float IndicatorBaseSize = 100.f;

	/** 포물선 최고 높이 = 수평거리 × 이 비율. 0 이면 직선. 거리에 비례해 일관된 호. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Missile", meta = (ClampMin = "0.0"))
	float ArcHeightRatio = 0.4f;

	/** 도착 시 폭발 파티클(Cascade). 비워두면 폭발 VFX 없음(보스의 BP_OnPattern2Impact 로 처리 가능). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Missile")
	TObjectPtr<UParticleSystem> ExplosionFX;

	/** 도착 시 폭발 사운드(선택). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PD|Missile")
	TObjectPtr<USoundBase> ExplosionSound;

	/** 폭발 연출 훅(데칼·추가 VFX 등). 데미지는 보스가 처리. */
	UFUNCTION(BlueprintImplementableEvent, Category = "PD|Missile")
	void BP_OnExploded(FVector Location);

private:
	void Explode();

	FVector StartLoc  = FVector::ZeroVector;
	FVector ImpactLoc = FVector::ZeroVector;
	float   TravelTime = 2.5f;
	float   Elapsed = 0.f;
	bool    bLaunched = false;
};
