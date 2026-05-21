#include "Items/PDLootBoxActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Items/PDLootComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"

APDLootBoxActor::APDLootBoxActor()
{
	// 거리 기반 하이라이트 스텐실 갱신 — 클라이언트 시각 효과라 Tick 으로 충분.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// LootBox 콘텐츠가 모든 클라에 동기화되도록 액터 자체 리플리케이션 활성화.
	bReplicates = true;

	InteractionCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionCollision"));
	SetRootComponent(InteractionCollision);
	InteractionCollision->SetBoxExtent(FVector(80.f, 80.f, 80.f));
	ConfigureInteractionCollision();

	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	BoxMesh->SetupAttachment(InteractionCollision);
	BoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LootComponent = CreateDefaultSubobject<UPDLootComponent>(TEXT("LootComponent"));
}

void APDLootBoxActor::BeginPlay()
{
	Super::BeginPlay();
	ConfigureInteractionCollision();

	// Overlay 머티리얼은 거리 안에 들어왔을 때만 부착 — 드로콜 비용 회피.
	// MID 는 한 번만 생성해두고 Update 에서 파라미터만 갱신.
	if (BoxMesh && OutlineMaterial)
	{
		OutlineMID = UMaterialInstanceDynamic::Create(OutlineMaterial, this);
	}
}

void APDLootBoxActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Throttle — HighlightUpdateInterval > 0 이면 누적해서 그 주기로만 갱신.
	if (HighlightUpdateInterval > 0.f)
	{
		HighlightAccumulator += DeltaSeconds;
		if (HighlightAccumulator < HighlightUpdateInterval) return;
		HighlightAccumulator = 0.f;
	}

	UpdateOutlineParams();
}

void APDLootBoxActor::UpdateOutlineParams()
{
	if (!BoxMesh || !OutlineMID) return;

	// 로컬 PlayerController 의 Pawn 기준으로 거리 평가. 멀티플 시 각 클라가 자기 시점으로 계산.
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		BoxMesh->SetOverlayMaterial(nullptr);
		return;
	}

	const float DistSq = FVector::DistSquared(GetActorLocation(), PlayerPawn->GetActorLocation());
	const float MaxDistSq = HighlightMaxDistance * HighlightMaxDistance;

	if (DistSq > MaxDistSq)
	{
		// 범위 밖 — Overlay 해제로 드로콜 자체 제거.
		BoxMesh->SetOverlayMaterial(nullptr);
		return;
	}

	// [MinDistance .. MaxDistance] → [1 .. 0] 알파 + Pow 비선형.
	const float Dist = FMath::Sqrt(DistSq);
	const float Range = FMath::Max(HighlightMaxDistance - HighlightMinDistance, KINDA_SMALL_NUMBER);
	const float LinearAlpha = FMath::Clamp((HighlightMaxDistance - Dist) / Range, 0.f, 1.f);
	const float Alpha = FMath::Pow(LinearAlpha, HighlightFalloffExponent);

	const float Intensity = FMath::Lerp(OutlineIntensityMin, OutlineIntensityMax, Alpha);
	const float Thickness = FMath::Lerp(OutlineThicknessMin, OutlineThicknessMax, Alpha);

	// 범위 재진입 시 Overlay 재부착 (이전 Tick 에서 nullptr 해제됐을 수 있음).
	if (BoxMesh->GetOverlayMaterial() != OutlineMID)
	{
		BoxMesh->SetOverlayMaterial(OutlineMID);
	}

	OutlineMID->SetScalarParameterValue(IntensityParamName, Intensity);
	OutlineMID->SetScalarParameterValue(ThicknessParamName, Thickness);
}

void APDLootBoxActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindLootClose();
	Super::EndPlay(EndPlayReason);
}

void APDLootBoxActor::Interact_Implementation(AActor* Interactor)
{
	APawn* InteractingPawn = Cast<APawn>(Interactor);
	if (!InteractingPawn) return;

	APDPlayerController* PlayerController = Cast<APDPlayerController>(InteractingPawn->GetController());
	if (!PlayerController) return;

	// 동일 박스 재클릭 → 닫기 토글.
	if (PlayerController->IsLootInterfaceOpen() && PlayerController->GetActiveLootComponent() == LootComponent)
	{
		PlayerController->CloseLootInterface();
		return;
	}

	PlayerController->OpenLootInterface(LootComponent);

	if (PlayerController->IsLootInterfaceOpen() && PlayerController->GetActiveLootComponent() == LootComponent)
	{
		BindLootClose(PlayerController);
		PlayInteractSound(true);
	}
}

void APDLootBoxActor::ConfigureInteractionCollision() const
{
	if (!InteractionCollision) return;

	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollision->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionCollision->SetGenerateOverlapEvents(true);
}

void APDLootBoxActor::PlayInteractSound(bool bOpen) const
{
	USoundBase* Sound = bOpen ? OpenSound.Get() : CloseSound.Get();
	if (!Sound) return;

	UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation(), SoundVolumeMultiplier);
}

void APDLootBoxActor::BindLootClose(APDPlayerController* PlayerController)
{
	if (!PlayerController) return;

	UnbindLootClose();
	BoundPlayerController = PlayerController;
	PlayerController->OnLootInterfaceClosed.AddUObject(this, &APDLootBoxActor::HandleLootInterfaceClosed);
}

void APDLootBoxActor::UnbindLootClose()
{
	if (BoundPlayerController.IsValid())
	{
		BoundPlayerController->OnLootInterfaceClosed.RemoveAll(this);
	}
	BoundPlayerController.Reset();
}

void APDLootBoxActor::HandleLootInterfaceClosed(UPDLootComponent* ClosedLootComponent)
{
	if (ClosedLootComponent != LootComponent) return;

	UnbindLootClose();
	PlayInteractSound(false);
}
