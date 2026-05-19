#include "Items/PDLootBoxActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Items/PDLootComponent.h"
#include "Kismet/GameplayStatics.h"
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

	// CustomDepth 렌더는 멀티 패스 비용이 있으므로 거리 안에 들어왔을 때만 켬.
	// 초기 상태는 꺼두고 Tick 에서 거리 평가 후 결정.
	if (BoxMesh)
	{
		BoxMesh->SetRenderCustomDepth(false);
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

	UpdateHighlightStencil();
}

void APDLootBoxActor::UpdateHighlightStencil()
{
	if (!BoxMesh) return;

	// 로컬 PlayerController 의 Pawn 기준으로 거리 평가. 멀티플 시 각 클라가 자기 시점으로 계산.
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		BoxMesh->SetRenderCustomDepth(false);
		return;
	}

	const float DistSq = FVector::DistSquared(GetActorLocation(), PlayerPawn->GetActorLocation());
	const float MaxDistSq = HighlightMaxDistance * HighlightMaxDistance;

	if (DistSq > MaxDistSq)
	{
		// 범위 밖 — CustomDepth 끄고 비용 0 으로.
		BoxMesh->SetRenderCustomDepth(false);
		return;
	}

	// [MinDistance .. MaxDistance] → [255 .. MinStencil] 선형 매핑.
	const float Dist = FMath::Sqrt(DistSq);
	const float Range = FMath::Max(HighlightMaxDistance - HighlightMinDistance, KINDA_SMALL_NUMBER);
	const float Alpha = FMath::Clamp((HighlightMaxDistance - Dist) / Range, 0.f, 1.f); // 1=가까움, 0=경계
	const int32 StencilValue = FMath::Lerp(HighlightMinStencil, 255, Alpha);

	BoxMesh->SetRenderCustomDepth(true);
	BoxMesh->SetCustomDepthStencilValue(StencilValue);
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
