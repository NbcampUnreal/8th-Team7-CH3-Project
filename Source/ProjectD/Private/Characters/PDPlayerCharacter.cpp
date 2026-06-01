#include "Characters/PDPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet/PDAttributeSet.h"
#include "Camera/CameraComponent.h"
#include "Component/PDVisionComponent.h"
#include "Component/PDInteractionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/PDGameInstance.h"
#include "Core/PDPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
<<<<<<< HEAD
#include "GameplayTag/PDGameplayTags.h"
#include "Items/Containers/PDQuickSlotComponent.h"
#include "Items/Equipment/PDEquipmentComponent.h"
#include "Items/Containers/PDInventoryComponent.h"
#include "Items/Equipment/PDEquipmentModificationComponent.h"
#include "Items/Containers/PDSecureContainerComponent.h"
#include "Interfaces/PDInteractable.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Animation/PDAnimInstance.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float PDPlayerCameraArmLength = 1333.333f;
	constexpr float PDPlayerCameraPitch = -60.f;

	bool IsInteractTargetInRange(const AActor* Interactor, const AActor* TargetActor, float MaxInteractDistance)
	{
		if (!Interactor || !TargetActor)
		{
			return false;
		}

		const FVector InteractorLocation = Interactor->GetActorLocation();
		const float MaxDistanceSq = FMath::Square(FMath::Max(0.f, MaxInteractDistance));
		if (FVector::DistSquared(InteractorLocation, TargetActor->GetActorLocation()) <= MaxDistanceSq)
		{
			return true;
		}

		const FBox Bounds = TargetActor->GetComponentsBoundingBox(true);
		return Bounds.IsValid && Bounds.ComputeSquaredDistanceToPoint(InteractorLocation) <= MaxDistanceSq;
	}

	void SendReviveCancelEvent(UAbilitySystemComponent* AbilitySystemComponent, AActor* Reviver, AActor* TargetActor)
	{
		if (!AbilitySystemComponent || !TargetActor)
		{
			return;
		}

		FGameplayEventData EventData;
		EventData.EventTag = PDGameplayTags::Event_Revive_Cancel;
		EventData.Instigator = Reviver;
		EventData.Target = TargetActor;
		AbilitySystemComponent->HandleGameplayEvent(PDGameplayTags::Event_Revive_Cancel, &EventData);
	}
}
=======
#include "Items/PDQuickSlotComponent.h"
#include "Weapons/PDWeaponBase.h"
>>>>>>> origin/main

APDPlayerCharacter::APDPlayerCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch=false;
	bUseControllerRotationYaw=false;
	bUseControllerRotationRoll=false;

	GetCharacterMovement()->bOrientRotationToMovement=false;
	GetCharacterMovement()->bUseControllerDesiredRotation=true;
	GetCharacterMovement()->RotationRate=FRotator(0.f, 10.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane=true;
	GetCharacterMovement()->bSnapToPlaneAtStart=true;
	GetCharacterMovement()->GravityScale=2.f;

	CameraBoom=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength=PDPlayerCameraArmLength;
	CameraBoom->SetRelativeRotation(FRotator(PDPlayerCameraPitch, 0.f, 0.f));
	CameraBoom->bDoCollisionTest=false;

	TopDownCameraComponent=CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation=false;

	VisionComponent=CreateDefaultSubobject<UPDVisionComponent>(TEXT("VisionComponent"));
	InteractionComponent=CreateDefaultSubobject<UPDInteractionComponent>(TEXT("InteractionComponent"));
	QuickSlotComponent=CreateDefaultSubobject<UPDQuickSlotComponent>(TEXT("QuickSlotComponent"));

	EquipmentModificationComponent=CreateDefaultSubobject<UPDEquipmentModificationComponent>(TEXT("EquipmentModificationComponent"));
	SecureContainerComponent=CreateDefaultSubobject<UPDSecureContainerComponent>(TEXT("SecureContainerComponent"));

	PrimaryActorTick.bCanEverTick=true;
	PrimaryActorTick.bStartWithTickEnabled=true;
	WeaponSlots.SetNum(4);



	TeamID = 1;
}

void APDPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APDPlayerCharacter, WeaponSlots);
	DOREPLIFETIME(APDPlayerCharacter, CurrentSlot);
	DOREPLIFETIME(APDPlayerCharacter, ReplicatedWeaponType);
	DOREPLIFETIME(APDPlayerCharacter, ReplicatedAimWorldLocation);
	DOREPLIFETIME(APDPlayerCharacter, bHasReplicatedAimWorldLocation);
}

void APDPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = PDPlayerCameraArmLength;
		CameraBoom->SetWorldRotation(FRotator(PDPlayerCameraPitch, 0.f, 0.f));
	}

	GetOrCreateInteractionComponent();

	if (HasAuthority())
	{
		UPDInventoryComponent* RuntimeInventoryComponent = GetInventoryComponent();
		UPDInventoryComponent* EditorInventoryComponent = FindComponentByClass<UPDInventoryComponent>();

		if (RuntimeInventoryComponent && EditorInventoryComponent && RuntimeInventoryComponent != EditorInventoryComponent)
		{
			RuntimeInventoryComponent->GridColumns = FMath::Max(1, EditorInventoryComponent->GridColumns);
			RuntimeInventoryComponent->GridRows = FMath::Max(1, EditorInventoryComponent->GridRows);
			RuntimeInventoryComponent->BaseCarryWeight = EditorInventoryComponent->BaseCarryWeight;

			if (EditorInventoryComponent->ItemDataTable)
			{
				RuntimeInventoryComponent->ItemDataTable = EditorInventoryComponent->ItemDataTable;
			}

			RuntimeInventoryComponent->InitializeInventory();
		}
	}

	 if (ASC)
        {
            ASC->RegisterGameplayTagEvent(PDGameplayTags::Weapon_Type_Rifle,   EGameplayTagEventType::NewOrRemoved).AddUObject(this, &APDPlayerCharacter::OnWeaponTypeTagChanged);
            ASC->RegisterGameplayTagEvent(PDGameplayTags::Weapon_Type_Shotgun, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &APDPlayerCharacter::OnWeaponTypeTagChanged);
            ASC->RegisterGameplayTagEvent(PDGameplayTags::Weapon_Type_Sniper,  EGameplayTagEventType::NewOrRemoved).AddUObject(this, &APDPlayerCharacter::OnWeaponTypeTagChanged);
            ASC->RegisterGameplayTagEvent(PDGameplayTags::Weapon_Type_Melee,   EGameplayTagEventType::NewOrRemoved).AddUObject(this, &APDPlayerCharacter::OnWeaponTypeTagChanged);
	        }
	LinkDefaultAnimLayer();
	GetCharacterMovement()->bAllowPhysicsRotationDuringAnimRootMotion = false;

	if (HasAuthority())
	{
		if (UPDGameInstance* GI=GetGameInstance<UPDGameInstance>())
		{
			if (GI->ConsumePendingResetToBase())
			{
				ResetToBase();
			}
		}
	}
}

UPDInteractionComponent* APDPlayerCharacter::GetOrCreateInteractionComponent()
{
	if (InteractionComponent)
	{
		return InteractionComponent;
	}

	InteractionComponent = FindComponentByClass<UPDInteractionComponent>();
	if (InteractionComponent)
	{
		return InteractionComponent;
	}

	InteractionComponent = NewObject<UPDInteractionComponent>(this, TEXT("InteractionComponent_Runtime"));
	if (InteractionComponent)
	{
		AddInstanceComponent(InteractionComponent);
		InteractionComponent->RegisterComponent();
	}

	return InteractionComponent;
}

UPDInventoryComponent* APDPlayerCharacter::GetInventoryComponent() const
{
	if (const APDPlayerState* PDPlayerState = GetPlayerState<APDPlayerState>())
	{
		return PDPlayerState->GetInventoryComponent();
	}

	return nullptr;
}

UPDQuickSlotComponent* APDPlayerCharacter::GetQuickSlotComponent() const
{
	if (const APDPlayerState* PDPlayerState = GetPlayerState<APDPlayerState>())
	{
		return PDPlayerState->GetQuickSlotComponent();
	}

	return nullptr;
}

UPDEquipmentComponent* APDPlayerCharacter::GetEquipmentComponent() const
{
	if (const APDPlayerState* PDPlayerState = GetPlayerState<APDPlayerState>())
	{
		return PDPlayerState->GetEquipmentComponent();
	}

	return nullptr;
}

bool APDPlayerCharacter::CanInteract_Implementation(AActor* Interactor) const
{
	// 플레이어는 쓰러진(Downed) 상태에서 Revive 대상일 때만 상호작용 프롬프트 노출.
	return IsDowned();
}

UPDQuestComponent* APDPlayerCharacter::GetQuestComponent() const
{
	if (const APDPlayerState* PDPlayerState = GetPlayerState<APDPlayerState>())
	{
		return PDPlayerState->GetQuestComponent();
	}

	return nullptr;
}

void APDPlayerCharacter::OnRep_WeaponSlots()
{
	SyncWeaponPresentation();
	OnWeaponSwapped.Broadcast(GetCurrentWeapon(), CurrentSlot);
}

void APDPlayerCharacter::OnRep_CurrentSlot()
{
	SyncWeaponPresentation();
	OnWeaponSwapped.Broadcast(GetCurrentWeapon(), CurrentSlot);
}

void APDPlayerCharacter::OnRep_ReplicatedWeaponType()
{
	SyncWeaponTypeTags(ReplicatedWeaponType);
	SyncWeaponPresentation();
	OnWeaponSwapped.Broadcast(GetCurrentWeapon(), CurrentSlot);
}

void APDPlayerCharacter::SyncWeaponPresentation()
{
	UPDAnimInstance* AnimInst = GetMesh()
		? Cast<UPDAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr;

	APDWeaponBase* CurrentWeapon = nullptr;
	const int32 CurrentIndex = CurrentSlot != EWeaponSlot::None
		? static_cast<int32>(CurrentSlot) : INDEX_NONE;

	for (int32 Index = 0; Index < WeaponSlots.Num(); ++Index)
	{
		APDWeaponBase* Weapon = WeaponSlots[Index];
		if (!IsValid(Weapon)) continue;

		const bool bIsCurrent = Index == CurrentIndex;

		Weapon->SetActorHiddenInGame(!bIsCurrent);
		if (bIsCurrent)
		{
			CurrentWeapon = Weapon;
		}
	}

	if (LifeState == EPDLifeState::Dead)
	{
		return;
	}
	if (LifeState == EPDLifeState::Downed)
	{
		LinkDownedAnimLayer();
		return;
	}
	if (LifeState == EPDLifeState::GettingUp)
	{
		return;
	}

	if (CurrentWeapon)
	{
		AttachActorToWeaponSocket(CurrentWeapon);
		if (USkeletalMeshComponent* WeaponMesh = CurrentWeapon->GetWeaponMesh())
		{
			WeaponMesh->SetVisibility(true, true);
			WeaponMesh->SetHiddenInGame(false);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		CurrentWeapon->SetActorHiddenInGame(false);
		ApplyWeaponAnimationLayer(CurrentWeapon);
		if (AnimInst)
		{
			AnimInst->OnWeaponEquipped(Cast<APDRangedWeaponBase>(CurrentWeapon));
		}
	}
	else
	{
		ApplyWeaponAnimationLayerForType(ReplicatedWeaponType);
	}
}

void APDPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CameraBoom)
		CameraBoom->SetWorldRotation(FRotator(PDPlayerCameraPitch, 0.f, 0.f));
}

void APDPlayerCharacter::InitAbilitySystem()
{
	Super::InitAbilitySystem();
	if (!ASC) return;

	if (!bPlayerAbilityDelegatesBound)
	{
		if (UPDVisionComponent* Vision=FindComponentByClass<UPDVisionComponent>())
		{
			Vision->BindToAttributeSet(ASC);
		}

		ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetStaminaAttribute())
			.AddUObject(this, &APDPlayerCharacter::OnStaminaChanged);
		ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetMoveSpeedAttribute())
			.AddUObject(this, &APDPlayerCharacter::OnMoveSpeedChangedForStaminaRegen);
		bPlayerAbilityDelegatesBound = true;
	}

	if (!HasAuthority() || bPlayerPersistentEffectsApplied)
	{
		return;
	}

	RefreshStaminaRegenEffects();

	HungerDecayHandle  = ApplyPlayerPersistentEffect(HungerDecayEffectClass);
	ThirstDecayHandle  = ApplyPlayerPersistentEffect(ThirstDecayEffectClass);
	GasMaskDecayHandle = ApplyPlayerPersistentEffect(GasMaskDecayEffectClass);
	bPlayerPersistentEffectsApplied = true;
}

void APDPlayerCharacter::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (!AttributeSet||!VisionComponent) return;
	const float MaxStamina=AttributeSet->GetMaxStamina();
	if (MaxStamina<=0.f) return;
	VisionComponent->UpdateStaminaScale(Data.NewValue/MaxStamina);
}


void APDPlayerCharacter::OnMoveSpeedChangedForStaminaRegen(const FOnAttributeChangeData&)
{
	RefreshStaminaRegenEffects();
}

FActiveGameplayEffectHandle APDPlayerCharacter::ApplyPlayerPersistentEffect(TSubclassOf<UGameplayEffect> GEClass)
{
	if (!ASC || !GEClass) return FActiveGameplayEffectHandle();

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GEClass, 1.f, Context);
	return Spec.IsValid() ? ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()) : FActiveGameplayEffectHandle();
}

bool APDPlayerCharacter::ShouldApplyStaminaRegen() const
{
	if (!AttributeSet)
	{
		return false;
	}

	return AttributeSet->GetMoveSpeed() <= StaminaRegenMaxMoveSpeed;
}

void APDPlayerCharacter::RefreshStaminaRegenEffects()
{
	if (!HasAuthority() || !ASC)
	{
		return;
	}

	const bool bShouldRegen = ShouldApplyStaminaRegen();
	if (bShouldRegen)
	{
		if (!StaminaRegenHandle.IsValid())
		{
			StaminaRegenHandle = ApplyPlayerPersistentEffect(StaminaRegenEffectClass);
		}
		if (!StaminaRegenBonusHandle.IsValid())
		{
			StaminaRegenBonusHandle = ApplyPlayerPersistentEffect(StaminaRegenBonusEffectClass);
		}
		return;
	}

	if (StaminaRegenHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(StaminaRegenHandle);
		StaminaRegenHandle.Invalidate();
	}
	if (StaminaRegenBonusHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(StaminaRegenBonusHandle);
		StaminaRegenBonusHandle.Invalidate();
	}
}
void APDPlayerCharacter::HandleDeath(AActor* Killer)
{
	if (IsDead()) return;

	if (APDWeaponBase* CurWeapon=GetCurrentWeapon())
	{
		if (UPDAnimInstance* AnimInst = GetMesh()
			? Cast<UPDAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr)
		{
			AnimInst->OnWeaponUnequipped(Cast<APDRangedWeaponBase>(CurWeapon));
		}
		CurWeapon->OnUnequip();
	}

	if (APlayerController* PC=Cast<APlayerController>(GetController()))
		PC->DisableInput(PC);

	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Super::HandleDeath(Killer);
}

bool APDPlayerCharacter::ShouldEnterDownedStateOnLethalDamage() const
{
	UWorld* World = GetWorld();
	if (!World) return false;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* OtherPC = It->Get();
		if (!OtherPC) continue;

		APDPlayerCharacter* OtherChar = Cast<APDPlayerCharacter>(OtherPC->GetPawn());
		if (!OtherChar || OtherChar == this) continue;

		if (OtherChar->GetLifeState() == EPDLifeState::Alive)
		{
			return true; // ?�른 Alive ?�료 존재 ??Downed 진입 (리바?�브 기�?)
		}
	}
	return false; // ?�른 Alive ?�료 ?�음 (Downed/Dead/Extracted �??�음) ??즉사
}

void APDPlayerCharacter::OnLifeStateChanged(EPDLifeState OldLifeState, AActor* ContextActor)
{
	Super::OnLifeStateChanged(OldLifeState, ContextActor);

	if (LifeState == EPDLifeState::Downed)
	{
		if (GetWorld())
		{
			GetWorldTimerManager().ClearTimer(GetUpTimerHandle);
		}
		LinkDownedAnimLayer();
		return;
	}

	if (LifeState == EPDLifeState::GettingUp)
	{
		BeginGettingUpPresentation();
		return;
	}

	if (LifeState == EPDLifeState::Alive)
	{
		if (GetWorld())
		{
			GetWorldTimerManager().ClearTimer(GetUpTimerHandle);
		}
		ApplyWeaponAnimationLayerForType(ReplicatedWeaponType);
	}
}

void APDPlayerCharacter::PickupWeapon(APDWeaponBase* Weapon)
{
	if (!HasAuthority())
	{
		ServerPickupWeapon(Weapon);
		return;
	}

	if (!Weapon) return;
	EWeaponSlot TargetSlot=GetSlotForWeaponType(Weapon->GetWeaponType());
	if (TargetSlot==EWeaponSlot::None) return;

	int32 Idx=static_cast<int32>(TargetSlot);

	if (WeaponSlots[Idx]) return;

	WeaponSlots[Idx]=Weapon;
	Weapon->OnEquip(this);
	Weapon->SetActorHiddenInGame(true);
	AttachActorToWeaponSocket(Weapon);
	OnWeaponPickedUp.Broadcast(Weapon);

	if (CurrentSlot==EWeaponSlot::None)
	{
		SwitchToSlot(TargetSlot);
	}

	Weapon->ForceNetUpdate();
	ForceNetUpdate();
}

void APDPlayerCharacter::SetSharedAimWorldLocation(const FVector& AimLocation)
{
	ReplicatedAimWorldLocation = AimLocation;
	bHasReplicatedAimWorldLocation = true;
}

bool APDPlayerCharacter::GetSharedAimWorldLocation(FVector& OutLocation) const
{
	if (!bHasReplicatedAimWorldLocation)
	{
		return false;
	}

	OutLocation = ReplicatedAimWorldLocation;
	return true;
}

FVector APDPlayerCharacter::GetSharedVisionForwardVector(const FVector& FromLocation) const
{
	FVector AimLocation;
	if (GetSharedAimWorldLocation(AimLocation))
	{
		FVector Direction = AimLocation - FromLocation;
		Direction.Z = 0.f;
		if (!Direction.IsNearlyZero())
		{
			return Direction.GetSafeNormal();
		}
	}

	return GetActorForwardVector();
}

void APDPlayerCharacter::SwitchToSlot(EWeaponSlot Slot)
{
	if (IsDowned() || IsGettingUp() || IsDead()) return;

	if (!HasAuthority())
	{
		ServerSwitchToSlot(Slot);
		return;
	}

	if (Slot==CurrentSlot)
	{
		SyncWeaponPresentation();
		return;
	}

	if (UPDQuickSlotComponent* PlayerQuickSlotComponent = GetQuickSlotComponent())
	{
		PlayerQuickSlotComponent->CancelConsumableUse();
	}

	int32 Idx=static_cast<int32>(Slot);
	if (!WeaponSlots.IsValidIndex(Idx)||!WeaponSlots[Idx]) return;

	UPDAnimInstance* AnimInst = GetMesh()
		? Cast<UPDAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr;

	if (APDWeaponBase* CurWeapon=GetCurrentWeapon())
	{
		if (AnimInst)
			AnimInst->OnWeaponUnequipped(Cast<APDRangedWeaponBase>(CurWeapon));
		CurWeapon->OnUnequip();
		CurWeapon->SetActorHiddenInGame(true);
	}

	CurrentSlot=Slot;
	APDWeaponBase* NewWeapon=WeaponSlots[Idx];
	NewWeapon->OnEquip(this);
	ReplicatedWeaponType = NewWeapon->GetWeaponType();
	SyncWeaponTypeTags(ReplicatedWeaponType);
	NewWeapon->SetActorHiddenInGame(false);
	SyncWeaponPresentation();
	OnWeaponSwapped.Broadcast(NewWeapon, Slot);
}

void APDPlayerCharacter::DropCurrentWeapon()
{
	if (IsDowned() || IsGettingUp() || IsDead()) return;

	if (!HasAuthority())
	{
		ServerDropCurrentWeapon();
		return;
	}

	APDWeaponBase* CurWeapon=GetCurrentWeapon();
	if (!CurWeapon) return;

	if (UPDAnimInstance* AnimInst = GetMesh()
		? Cast<UPDAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr)
	{
		AnimInst->OnWeaponUnequipped(Cast<APDRangedWeaponBase>(CurWeapon));
	}
	CurWeapon->OnUnequip();
	CurWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	CurWeapon->SetDropped(true);
	WeaponSlots[static_cast<int32>(CurrentSlot)]=nullptr;
	CurrentSlot=EWeaponSlot::None;
	ReplicatedWeaponType = EWeaponType::None;
	SyncWeaponTypeTags(ReplicatedWeaponType);
	SyncWeaponPresentation();

	OnWeaponSwapped.Broadcast(nullptr, EWeaponSlot::None);
}

APDWeaponBase* APDPlayerCharacter::GetCurrentWeapon() const
{
	if (CurrentSlot==EWeaponSlot::None) return nullptr;
	int32 Idx=static_cast<int32>(CurrentSlot);
	return WeaponSlots.IsValidIndex(Idx) ? WeaponSlots[Idx].Get() : nullptr;
}

APDWeaponBase* APDPlayerCharacter::GetWeaponInSlot(EWeaponSlot Slot) const
{
	int32 Idx=static_cast<int32>(Slot);
	return WeaponSlots.IsValidIndex(Idx) ? WeaponSlots[Idx].Get() : nullptr;
}

EWeaponSlot APDPlayerCharacter::GetSlotForWeaponType(EWeaponType Type) const
{
	switch (Type)
	{
	case EWeaponType::Rifle:   return EWeaponSlot::Slot1_Rifle;
	case EWeaponType::Shotgun: return EWeaponSlot::Slot2_Shotgun;
	case EWeaponType::Sniper:  return EWeaponSlot::Slot3_Sniper;
	case EWeaponType::Melee:   return EWeaponSlot::Slot4_Melee;
	default:                   return EWeaponSlot::None;
	}
}

bool APDPlayerCharacter::TryAutoEquipWeaponItem(const FPDItemData& ItemData)
{
	if (!HasAuthority()) return false;

	FPDInventorySlot TempSlot;
	TempSlot.ItemData = ItemData;
	TempSlot.Quantity = 1;
	TempSlot.bIsEmpty = false;
	TempSlot.ModificationLevel = 0;
	TempSlot.EnsureInstanceID();
	return TryAutoEquipWeaponSlot(TempSlot);
}

bool APDPlayerCharacter::TryAutoEquipWeaponSlot(const FPDInventorySlot& ItemSlot)
{
	if (!HasAuthority()) return false;

	const FPDItemData& ItemData = ItemSlot.ItemData;
	if (!ItemData.WeaponClass) return false;

	const EWeaponSlot TargetSlot = GetSlotForWeaponType(ItemData.WeaponType);
	if (TargetSlot == EWeaponSlot::None) return false;


	if (GetWeaponInSlot(TargetSlot)) return false;

	UWorld* World = GetWorld();
	if (!World) return false;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APDWeaponBase* SpawnedWeapon = World->SpawnActor<APDWeaponBase>(
		ItemData.WeaponClass, GetActorTransform(), SpawnParams);
	if (!SpawnedWeapon) return false;

	SpawnedWeapon->ItemID = ItemData.ItemID;
	SpawnedWeapon->ItemInstanceID = ItemSlot.ItemInstanceID;
	if (SpawnedWeapon->WeaponType == EWeaponType::None)
	{
		SpawnedWeapon->WeaponType = ItemData.WeaponType;
	}

	SpawnedWeapon->SetLevel(FMath::Max(1, ItemSlot.ModificationLevel + 1));

	// ???�속?�된 ?�탄??PickupWeapon ?�출 ?�에 ?�용 ??PickupWeapon ?��???SwitchToSlot??	//    OnWeaponSwapped�?broadcast?�기 ?�에 actor??CurrentAmmo가 ?�확??값이?�야
	//    UI가 �??�시부???�바�??�탄??보임.
	if (APDRangedWeaponBase* Ranged = Cast<APDRangedWeaponBase>(SpawnedWeapon))
	{
		if (ItemSlot.WeaponState.HasPersistedAmmo())
		{
			Ranged->SetCurrentAmmo(ItemSlot.WeaponState.CurrentAmmo);
		}
	}

	PickupWeapon(SpawnedWeapon);
	SpawnedWeapon->ForceNetUpdate();
	ForceNetUpdate();
	return true;
}


bool APDPlayerCharacter::RemoveEquippedWeaponItem(const FPDItemData& ItemData, bool bDestroyWeaponActor)
{
	FPDWeaponInstanceState Discarded;
	return RemoveEquippedWeaponItemPreservingState(ItemData, Discarded, bDestroyWeaponActor);
}

bool APDPlayerCharacter::RemoveEquippedWeaponItemByInstanceID(const FGuid& ItemInstanceID, bool bDestroyWeaponActor)
{
	FPDWeaponInstanceState Discarded;
	return RemoveEquippedWeaponItemByInstanceIDPreservingState(ItemInstanceID, Discarded, bDestroyWeaponActor);
}

bool APDPlayerCharacter::RemoveEquippedWeaponItemPreservingState(const FPDItemData& ItemData,
                                                                  FPDWeaponInstanceState& OutState,
                                                                  bool bDestroyWeaponActor)
{
	if (!HasAuthority()) return false;

	const EWeaponSlot TargetSlot = GetSlotForWeaponType(ItemData.WeaponType);
	return RemoveEquippedWeaponInSlotPreservingState(TargetSlot, OutState, bDestroyWeaponActor);
}

bool APDPlayerCharacter::RemoveEquippedWeaponItemByInstanceIDPreservingState(const FGuid& ItemInstanceID,
                                                                             FPDWeaponInstanceState& OutState,
                                                                             bool bDestroyWeaponActor)
{
	if (!HasAuthority() || !ItemInstanceID.IsValid())
	{
		return false;
	}

	for (int32 SlotIndex = 0; SlotIndex < WeaponSlots.Num(); ++SlotIndex)
	{
		APDWeaponBase* Weapon = WeaponSlots[SlotIndex].Get();
		if (Weapon && Weapon->GetItemInstanceID() == ItemInstanceID)
		{
			return RemoveEquippedWeaponInSlotPreservingState(
				static_cast<EWeaponSlot>(SlotIndex),
				OutState,
				bDestroyWeaponActor);
		}
	}

	return false;
}

bool APDPlayerCharacter::RemoveEquippedWeaponInSlotPreservingState(EWeaponSlot TargetSlot,
                                                                    FPDWeaponInstanceState& OutState,
                                                                    bool bDestroyWeaponActor)
{
	if (!HasAuthority() || TargetSlot == EWeaponSlot::None)
	{
		return false;
	}

	const int32 SlotIndex = static_cast<int32>(TargetSlot);
	if (!WeaponSlots.IsValidIndex(SlotIndex) || !WeaponSlots[SlotIndex])
	{
		return false;
	}

	APDWeaponBase* WeaponToRemove = WeaponSlots[SlotIndex];

	// ??destroy ?�전???��????�태 추출 ???�출?��? ?�벤?�리 ?�롯??stamp ??
	if (APDRangedWeaponBase* Ranged = Cast<APDRangedWeaponBase>(WeaponToRemove))
	{
		OutState.CurrentAmmo = Ranged->GetCurrentAmmo();
	}

	WeaponToRemove->OnUnequip();
	WeaponToRemove->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	WeaponToRemove->SetActorHiddenInGame(true);

	if (ASC)
	{
		if (ASC->HasMatchingGameplayTag(WeaponToRemove->GetWeaponTypeTag()))
		{
			ASC->RemoveLooseGameplayTag(WeaponToRemove->GetWeaponTypeTag());
		}
	}

	WeaponSlots[SlotIndex] = nullptr;
	const bool bWasCurrent = (CurrentSlot == TargetSlot);
	if (bWasCurrent)
	{
		CurrentSlot = EWeaponSlot::None;
		ReplicatedWeaponType = EWeaponType::None;
		SyncWeaponTypeTags(ReplicatedWeaponType);
		OnWeaponSwapped.Broadcast(nullptr, EWeaponSlot::None);
	}
	SyncWeaponPresentation();

	if (bDestroyWeaponActor)
	{
		WeaponToRemove->Destroy();
	}

	// 메인 무기가 비워�??�실??UI/Anim???�림. ?�왑 ?�름???��??�면 Apply가 �???무기�??�시 broadcast??
	if (bWasCurrent)
	{
		OnWeaponSwapped.Broadcast(nullptr, EWeaponSlot::None);
	}

	return true;
}

void APDPlayerCharacter::ResetToBase()
{
	if (!HasAuthority()) return;
	if (!ASC || !AttributeSet) return;

	ASC->RemoveActiveGameplayEffect(StaminaRegenHandle);
	ASC->RemoveActiveGameplayEffect(StaminaRegenBonusHandle);
	StaminaRegenHandle.Invalidate();
	StaminaRegenBonusHandle.Invalidate();
	ASC->RemoveActiveGameplayEffect(HungerDecayHandle);
	ASC->RemoveActiveGameplayEffect(ThirstDecayHandle);
	ASC->RemoveActiveGameplayEffect(GasMaskDecayHandle);

	auto RemoveDebuff = [&](const FGameplayTag& Tag)
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(Tag);
		ASC->RemoveActiveEffectsWithTags(Tags);
	};

	RemoveDebuff(PDGameplayTags::State_Debuff_Starving);
	RemoveDebuff(PDGameplayTags::State_Debuff_Dehydrated);
	RemoveDebuff(PDGameplayTags::State_Debuff_GasExposure);

	RemoveDebuff(PDGameplayTags::State_Debuff_Bleeding);
	RemoveDebuff(PDGameplayTags::State_Debuff_LegDamaged);
	RemoveDebuff(PDGameplayTags::State_Debuff_LegCrippled);
	RemoveDebuff(PDGameplayTags::State_Debuff_ArmDamaged);
	RemoveDebuff(PDGameplayTags::State_Debuff_ArmCrippled);

	ASC->SetNumericAttributeBase(UPDAttributeSet::GetHeadHPAttribute(),  AttributeSet->GetMaxHeadHP());
	ASC->SetNumericAttributeBase(UPDAttributeSet::GetTorsoHPAttribute(), AttributeSet->GetMaxTorsoHP());
	ASC->SetNumericAttributeBase(UPDAttributeSet::GetArmLHPAttribute(),  AttributeSet->GetMaxArmLHP());
	ASC->SetNumericAttributeBase(UPDAttributeSet::GetArmRHPAttribute(),  AttributeSet->GetMaxArmRHP());
	ASC->SetNumericAttributeBase(UPDAttributeSet::GetLegLHPAttribute(),  AttributeSet->GetMaxLegLHP());
	ASC->SetNumericAttributeBase(UPDAttributeSet::GetLegRHPAttribute(),  AttributeSet->GetMaxLegRHP());
	ASC->SetNumericAttributeBase(UPDAttributeSet::GetStaminaAttribute(), AttributeSet->GetMaxStamina());
	ASC->SetNumericAttributeBase(UPDAttributeSet::GetHungerAttribute(),  AttributeSet->GetMaxHunger());
	ASC->SetNumericAttributeBase(UPDAttributeSet::GetThirstAttribute(),  AttributeSet->GetMaxThirst());
	ASC->SetNumericAttributeBase(UPDAttributeSet::GetGasMaskAttribute(), AttributeSet->GetMaxGasMask());

	RefreshStaminaRegenEffects();
	HungerDecayHandle=ApplyPlayerPersistentEffect(HungerDecayEffectClass);
	ThirstDecayHandle=ApplyPlayerPersistentEffect(ThirstDecayEffectClass);
	GasMaskDecayHandle=ApplyPlayerPersistentEffect(GasMaskDecayEffectClass);

	ResetLifeStateToAlive(this);
}

void APDPlayerCharacter::TryInteract()
{
	if (IsDowned() || IsGettingUp() || IsDead())
	{
		return;
	}

	if (UPDQuickSlotComponent* PlayerQuickSlotComponent = GetQuickSlotComponent())
	{
		PlayerQuickSlotComponent->CancelConsumableUse();
	}

	if (!HasAuthority())
	{
		UPDInteractionComponent* ActiveInteractionComponent = GetOrCreateInteractionComponent();
		AActor* TargetActor = ActiveInteractionComponent ? ActiveInteractionComponent->FindInteractTarget() : nullptr;
		ActiveInteractTarget = TargetActor;
		ServerInteractTarget(TargetActor);
		return;
	}

	if (UPDInteractionComponent* ActiveInteractionComponent = GetOrCreateInteractionComponent())
	{
		AActor* TargetActor = ActiveInteractionComponent->FindInteractTarget();
		ActiveInteractTarget = TargetActor;
		if (IsValid(TargetActor) && TargetActor->GetClass()->ImplementsInterface(UPDInteractable::StaticClass()))
		{
			IPDInteractable::Execute_Interact(TargetActor, this);
		}
	}
}

void APDPlayerCharacter::StopInteract()
{
	AActor* TargetActor = ActiveInteractTarget.Get();
	ActiveInteractTarget.Reset();

	if (!HasAuthority())
	{
		ServerStopInteract(TargetActor);
		return;
	}

	if (APDCharacterBase* DownedCharacter = Cast<APDCharacterBase>(TargetActor))
	{
		SendReviveCancelEvent(ASC, this, DownedCharacter);

	}
}

bool APDPlayerCharacter::IsInteractingWith(const AActor* TargetActor) const
{
	return TargetActor && ActiveInteractTarget.Get() == TargetActor;
}

void APDPlayerCharacter::ServerTryInteract_Implementation()
{
	TryInteract();
}

void APDPlayerCharacter::ServerInteractTarget_Implementation(AActor* TargetActor)
{
	if (UPDQuickSlotComponent* PlayerQuickSlotComponent = GetQuickSlotComponent())
	{
		PlayerQuickSlotComponent->CancelConsumableUse();
	}

	if (IsValid(TargetActor) && TargetActor->GetClass()->ImplementsInterface(UPDInteractable::StaticClass()))
	{
		UPDInteractionComponent* ActiveInteractionComponent = GetOrCreateInteractionComponent();
		const float MaxInteractDistance = ActiveInteractionComponent
			? ActiveInteractionComponent->InteractDistance + 150.f : 450.f;

		if (IsInteractTargetInRange(this, TargetActor, MaxInteractDistance))
		{
			ActiveInteractTarget = TargetActor;
			IPDInteractable::Execute_Interact(TargetActor, this);
			return;
		}
	}

	if (UPDInteractionComponent* ActiveInteractionComponent = GetOrCreateInteractionComponent())
	{
		AActor* FallbackTargetActor = ActiveInteractionComponent->FindInteractTarget();
		ActiveInteractTarget = FallbackTargetActor;
		if (IsValid(FallbackTargetActor) && FallbackTargetActor->GetClass()->ImplementsInterface(UPDInteractable::StaticClass()))
		{
			IPDInteractable::Execute_Interact(FallbackTargetActor, this);
		}
	}
}

void APDPlayerCharacter::ServerStopInteract_Implementation(AActor* TargetActor)
{
	if (APDCharacterBase* DownedCharacter = Cast<APDCharacterBase>(TargetActor))
	{
		SendReviveCancelEvent(ASC, this, DownedCharacter);

	}
}

void APDPlayerCharacter::ServerHandleAnimGameplayEvent_Implementation(FGameplayTag EventTag)
{
	if (!EventTag.IsValid() || !ASC)
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag = EventTag;
	EventData.Instigator = this;
	EventData.Target = this;
	ASC->HandleGameplayEvent(EventTag, &EventData);
}

void APDPlayerCharacter::ServerPickupWeapon_Implementation(APDWeaponBase* Weapon)
{
	PickupWeapon(Weapon);
}

void APDPlayerCharacter::ServerSwitchToSlot_Implementation(EWeaponSlot Slot)
{
	SwitchToSlot(Slot);
}

void APDPlayerCharacter::ServerDropCurrentWeapon_Implementation()
{
	DropCurrentWeapon();
}

void APDPlayerCharacter::OnWeaponTypeTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount==0) return;
	if (LifeState == EPDLifeState::Downed || LifeState == EPDLifeState::GettingUp)
	{
		return;
	}
	if (LifeState == EPDLifeState::Dead) return;

	ApplyWeaponAnimationLayer(GetCurrentWeapon());
}

void APDPlayerCharacter::ApplyWeaponAnimationLayer(APDWeaponBase* Weapon)
{
	if (!IsValid(Weapon))
	{
		ApplyWeaponAnimationLayerForType(ReplicatedWeaponType);
		return;
	}

	TSubclassOf<UAnimInstance> LayerClass=Weapon->GetWeaponAnimLayerClass();
	if (!LayerClass)
	{
		ApplyWeaponAnimationLayerForType(Weapon->GetWeaponType());
		return;
	}
	if (USkeletalMeshComponent* SkelMesh=GetMesh())
		SkelMesh->LinkAnimClassLayers(LayerClass);
}

void APDPlayerCharacter::ApplyAnimationLayerForTag(const FGameplayTag& LayerTag)
{
	if (const TSubclassOf<UAnimInstance>* LayerClass = WeaponAnimLayerMap.Find(LayerTag))
	{
		if (*LayerClass)
		{
			if (USkeletalMeshComponent* SkelMesh = GetMesh())
			{
				SkelMesh->LinkAnimClassLayers(*LayerClass);
				return;
			}
		}
	}

	LinkDefaultAnimLayer();
}

void APDPlayerCharacter::ApplyWeaponAnimationLayerForType(EWeaponType WeaponType)
{
	FGameplayTag LayerTag;
	switch (WeaponType)
	{
	case EWeaponType::Rifle:
		LayerTag = PDGameplayTags::Weapon_Type_Rifle;
		break;
	case EWeaponType::Shotgun:
		LayerTag = PDGameplayTags::Weapon_Type_Shotgun;
		break;
	case EWeaponType::Sniper:
		LayerTag = PDGameplayTags::Weapon_Type_Sniper;
		break;
	case EWeaponType::Melee:
		LayerTag = PDGameplayTags::Weapon_Type_Melee;
		break;
	default:
		LinkDefaultAnimLayer();
		return;
	}

	ApplyAnimationLayerForTag(LayerTag);
}

void APDPlayerCharacter::SyncWeaponTypeTags(EWeaponType WeaponType)
{
	if (!ASC)
	{
		return;
	}

	auto RemoveLooseTagIfPresent = [this](const FGameplayTag& Tag)
	{
		if (ASC->GetTagCount(Tag) > 0)
		{
			ASC->RemoveLooseGameplayTag(Tag);
		}
	};

	RemoveLooseTagIfPresent(PDGameplayTags::Weapon_Type_Rifle);
	RemoveLooseTagIfPresent(PDGameplayTags::Weapon_Type_Shotgun);
	RemoveLooseTagIfPresent(PDGameplayTags::Weapon_Type_Sniper);
	RemoveLooseTagIfPresent(PDGameplayTags::Weapon_Type_Melee);

	switch (WeaponType)
	{
	case EWeaponType::Rifle:
		ASC->AddLooseGameplayTag(PDGameplayTags::Weapon_Type_Rifle);
		break;
	case EWeaponType::Shotgun:
		ASC->AddLooseGameplayTag(PDGameplayTags::Weapon_Type_Shotgun);
		break;
	case EWeaponType::Sniper:
		ASC->AddLooseGameplayTag(PDGameplayTags::Weapon_Type_Sniper);
		break;
	case EWeaponType::Melee:
		ASC->AddLooseGameplayTag(PDGameplayTags::Weapon_Type_Melee);
		break;
	default:
		break;
	}
}

void APDPlayerCharacter::LinkDefaultAnimLayer()
{
	if (!DefaultAnimLayerClass) return;
	if (USkeletalMeshComponent* SkelMesh=GetMesh())
		SkelMesh->LinkAnimClassLayers(DefaultAnimLayerClass);
}

void APDPlayerCharacter::LinkDownedAnimLayer()
{
	TSubclassOf<UAnimInstance> LayerClass = DownedAnimLayerClass;
	if (!LayerClass)
	{
		if (const TSubclassOf<UAnimInstance>* MappedLayerClass = WeaponAnimLayerMap.Find(PDGameplayTags::State_Downed))
		{
			LayerClass = *MappedLayerClass;
		}
	}

	if (!LayerClass)
	{
		LinkDefaultAnimLayer();
		return;
	}

	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->LinkAnimClassLayers(LayerClass);
	}
}

void APDPlayerCharacter::BeginGettingUpPresentation()
{

	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(GetUpTimerHandle);
	}

	float GetUpDuration = 0.f;
	if (UPDAnimInstance* AnimInst = GetMesh() ? Cast<UPDAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr)
	{
		AnimInst->PlayGetUpMontage();
		GetUpDuration = AnimInst->GetGetUpMontageDuration();
	}
	else
	{
	}

	if (HasAuthority())
	{
		if (GetUpDuration <= KINDA_SMALL_NUMBER)
		{
			FinishGettingUp();
			return;
		}

		GetWorldTimerManager().SetTimer(
			GetUpTimerHandle,
			this,
			&APDPlayerCharacter::FinishGettingUpFromTimer,
			GetUpDuration,
			false);

	}
}

void APDPlayerCharacter::FinishGettingUpFromTimer()
{
	FinishGettingUp();
}
