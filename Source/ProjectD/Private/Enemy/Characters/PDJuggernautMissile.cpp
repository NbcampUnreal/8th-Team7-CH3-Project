#include "Enemy/Characters/PDJuggernautMissile.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

APDJuggernautMissile::APDJuggernautMissile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;   // 머신별 로컬 코스메틱 — 복제 불필요.

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	MissileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MissileMesh"));
	MissileMesh->SetupAttachment(RootScene);
	MissileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 착탄 인디케이터 — 절대 위치/회전/스케일로 부모(비행하는 루트) 이동과 분리해 바닥에 고정.
	ImpactIndicator = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ImpactIndicator"));
	ImpactIndicator->SetupAttachment(RootScene);
	ImpactIndicator->SetUsingAbsoluteLocation(true);
	ImpactIndicator->SetUsingAbsoluteRotation(true);
	ImpactIndicator->SetUsingAbsoluteScale(true);
	ImpactIndicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ImpactIndicator->SetVisibility(false);
}

void APDJuggernautMissile::InitMissile(const FVector& InStart, const FVector& InImpact, float InTravelTime, float InImpactRadius)
{
	StartLoc   = InStart;
	ImpactLoc  = InImpact;
	TravelTime = FMath::Max(InTravelTime, 0.05f);
	Elapsed    = 0.f;
	SetActorLocation(StartLoc);
	bLaunched  = true;

	// 착탄 인디케이터: 바닥(착탄지점)에 평면 배치 + 반경에 맞춰 스케일 + 차오름 시작.
	if (ImpactIndicator && ImpactIndicator->GetStaticMesh())
	{
		// BP 템플릿이 절대 transform 플래그를 덮어쓸 수 있어 런타임에 재확정 — 비행 루트와 분리해 바닥 고정.
		ImpactIndicator->SetUsingAbsoluteLocation(true);
		ImpactIndicator->SetUsingAbsoluteRotation(true);
		ImpactIndicator->SetUsingAbsoluteScale(true);

		ImpactIndicator->SetWorldLocation(ImpactLoc + FVector(0.f, 0.f, 2.f)); // z-fighting 회피 살짝 띄움
		ImpactIndicator->SetWorldRotation(FRotator::ZeroRotator);              // 평면(XY)이라 수평
		const float S = (IndicatorBaseSize > KINDA_SMALL_NUMBER)
			? (2.f * InImpactRadius / IndicatorBaseSize) : 1.f;                 // 지름 = 반경×2
		ImpactIndicator->SetWorldScale3D(FVector(S, S, 1.f));
		ImpactIndicator->SetVisibility(true);

		if (UMaterialInstanceDynamic* DMI = ImpactIndicator->CreateDynamicMaterialInstance(0))
		{
			const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
			DMI->SetScalarParameterValue(TEXT("StartTime"), Now);
			DMI->SetScalarParameterValue(TEXT("Duration"), TravelTime);
		}
	}
}

void APDJuggernautMissile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bLaunched) return;

	Elapsed += DeltaSeconds;
	const float T = FMath::Clamp(Elapsed / TravelTime, 0.f, 1.f);

	// 포물선: 직선 보간 + 수평거리 비례 호(sin 으로 t=0,1 에서 0, t=0.5 에서 최고).
	const FVector Flat = FMath::Lerp(StartLoc, ImpactLoc, T);
	const double  HorizDist = FVector::Dist2D(StartLoc, ImpactLoc);
	const double  Arc = HorizDist * ArcHeightRatio * FMath::Sin(PI * T);
	const FVector NewPos = Flat + FVector(0.f, 0.f, Arc);

	// 진행 방향으로 노즈 정렬(호를 따라 위→아래).
	const FVector Delta = NewPos - GetActorLocation();
	if (!Delta.IsNearlyZero())
	{
		SetActorRotation(Delta.Rotation());
	}
	SetActorLocation(NewPos);

	if (T >= 1.f)
	{
		Explode();
	}
}

void APDJuggernautMissile::Explode()
{
	bLaunched = false;

	if (ExplosionFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, ExplosionFX, ImpactLoc);
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, ImpactLoc);
	}

	BP_OnExploded(ImpactLoc);
	Destroy();
}
