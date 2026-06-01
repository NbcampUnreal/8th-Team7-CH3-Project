#include "Items/World/PDLootBoxActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/PDPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Items/Containers/PDLootComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"

APDLootBoxActor::APDLootBoxActor()
{
	// 거리 기반 ?�이?�이???�텐??갱신 ???�라?�언???�각 ?�과??Tick ?�로 충분.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// LootBox 콘텐츠�? 모든 ?�라???�기?�되?�록 ?�터 ?�체 리플리�??�션 ?�성??
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

	// Super::BeginPlay 이후라 LootComponent 초기화 완료 — 이 시점에 테이블을 굴려 채움.
	GenerateLootFromTable();

	// Overlay 머티리얼?� 거리 ?�에 ?�어?�을 ?�만 부�????�로�?비용 ?�피.
	// MID ????번만 ?�성?�두�?Update ?�서 ?�라미터�?갱신.
	if (BoxMesh && OutlineMaterial)
	{
		OutlineMID = UMaterialInstanceDynamic::Create(OutlineMaterial, this);
	}
}

void APDLootBoxActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Throttle ??HighlightUpdateInterval > 0 ?�면 ?�적?�서 �?주기로만 갱신.
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

	// 로컬 PlayerController ??Pawn 기�??�로 거리 ?��?. 멀?�플 ??�??�라가 ?�기 ?�점?�로 계산.
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
		// 범위 �???Overlay ?�제�??�로�??�체 ?�거.
		BoxMesh->SetOverlayMaterial(nullptr);
		return;
	}

	// [MinDistance .. MaxDistance] ??[1 .. 0] ?�파 + Pow 비선??
	const float Dist = FMath::Sqrt(DistSq);
	const float Range = FMath::Max(HighlightMaxDistance - HighlightMinDistance, KINDA_SMALL_NUMBER);
	const float LinearAlpha = FMath::Clamp((HighlightMaxDistance - Dist) / Range, 0.f, 1.f);
	const float Alpha = FMath::Pow(LinearAlpha, HighlightFalloffExponent);

	const float Intensity = FMath::Lerp(OutlineIntensityMin, OutlineIntensityMax, Alpha);
	const float Thickness = FMath::Lerp(OutlineThicknessMin, OutlineThicknessMax, Alpha);

	// 범위 ?�진????Overlay ?��?�?(?�전 Tick ?�서 nullptr ?�제?�을 ???�음).
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

void APDLootBoxActor::GenerateLootFromTable()
{
	// 로컬 생성 방지: 서버에서만 굴리고 LootItems 복제로 클라 동기화. 빈 테이블(적 시체 박스)은 즉시 반환.
	if (!HasAuthority() || !LootComponent || LootTable.Num() == 0) return;

	for (const FPDLootEntry& Entry : LootTable)
	{
		// 박스 그리드는 ItemID 기반(AddItemByID). ItemClass 전용 항목은 월드 드랍용이라 박스엔 부적합.
		if (Entry.ItemID.IsNone()) continue;
		if (FMath::FRand() > Entry.DropChance) continue;

		const int32 MinQ = FMath::Max(1, Entry.MinQuantity);
		const int32 MaxQ = FMath::Max(MinQ, Entry.MaxQuantity);
		const int32 Quantity = FMath::RandRange(MinQ, MaxQ);

		LootComponent->AddItemByID(Entry.ItemID, Quantity);
	}
}

void APDLootBoxActor::Interact_Implementation(AActor* Interactor)
{
	APawn* InteractingPawn = Cast<APawn>(Interactor);
	if (!InteractingPawn) return;

	APDPlayerController* PlayerController = Cast<APDPlayerController>(InteractingPawn->GetController());
	if (!PlayerController) return;

	// ?�일 박스 ?�클�????�기 ?��?.
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
