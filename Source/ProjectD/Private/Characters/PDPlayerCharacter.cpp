#include "Characters/PDPlayerCharacter.h"
#include "AttributeSet/PDAttributeSet.h"
#include "Camera/CameraComponent.h"
#include "Component/PDVisionComponent.h"
#include "Component/PDInteractionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/PDGameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Items/PDQuickSlotComponent.h"
#include "Items/PDEquipmentComponent.h"
#include "Items/PDEquipmentModificationComponent.h"
#include "Weapons/Base/PDWeaponBase.h"
#include "Weapons/Base/PDRangedWeaponBase.h"
#include "Animation/PDAnimInstance.h"

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
	CameraBoom->TargetArmLength=800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest=false;

	TopDownCameraComponent=CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation=false;

	VisionComponent=CreateDefaultSubobject<UPDVisionComponent>(TEXT("VisionComponent"));
	InteractionComponent=CreateDefaultSubobject<UPDInteractionComponent>(TEXT("InteractionComponent"));

	QuickSlotComponent=CreateDefaultSubobject<UPDQuickSlotComponent>(TEXT("QuickSlotComponent"));
	EquipmentComponent=CreateDefaultSubobject<UPDEquipmentComponent>(TEXT("EquipmentComponent"));
	EquipmentModificationComponent=CreateDefaultSubobject<UPDEquipmentModificationComponent>(TEXT("EquipmentModificationComponent"));

	PrimaryActorTick.bCanEverTick=true;
	PrimaryActorTick.bStartWithTickEnabled=true;
	WeaponSlots.SetNum(4);

	
	// Player 팀. AI 의 GetTeamAttitudeTowards 에서 적대 판정의 기준이 됨.
	TeamID = 1;
}

void APDPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	 if (ASC)
        {
            ASC->RegisterGameplayTagEvent(PDGameplayTags::Weapon_Type_Rifle,   EGameplayTagEventType::NewOrRemoved).AddUObject(this, &APDPlayerCharacter::OnWeaponTypeTagChanged);
            ASC->RegisterGameplayTagEvent(PDGameplayTags::Weapon_Type_Shotgun, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &APDPlayerCharacter::OnWeaponTypeTagChanged);
            ASC->RegisterGameplayTagEvent(PDGameplayTags::Weapon_Type_Sniper,  EGameplayTagEventType::NewOrRemoved).AddUObject(this, &APDPlayerCharacter::OnWeaponTypeTagChanged);
            ASC->RegisterGameplayTagEvent(PDGameplayTags::Weapon_Type_Melee,   EGameplayTagEventType::NewOrRemoved).AddUObject(this, &APDPlayerCharacter::OnWeaponTypeTagChanged);
        }
	LinkDefaultAnimLayer();
	GetCharacterMovement()->bAllowPhysicsRotationDuringAnimRootMotion = false;

	if (UPDGameInstance* GI=GetGameInstance<UPDGameInstance>())
	{
		if (GI->ConsumePendingResetToBase())
		{
			ResetToBase();
		}
	}
}

void APDPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CameraBoom)
		CameraBoom->SetWorldRotation(FRotator(-60.f, 0.f, 0.f));
}

void APDPlayerCharacter::InitAbilitySystem()
{
	Super::InitAbilitySystem();

	auto ApplyGE=[&](TSubclassOf<UGameplayEffect> GEClass) -> FActiveGameplayEffectHandle
	{
		if (!GEClass) return FActiveGameplayEffectHandle();
		FGameplayEffectContextHandle Context=ASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec=ASC->MakeOutgoingSpec(GEClass, 1.f, Context);
		if (Spec.IsValid())
			return ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		return FActiveGameplayEffectHandle();
	};

	if (UPDVisionComponent* Vision=FindComponentByClass<UPDVisionComponent>())
	{
		Vision->BindToAttributeSet(ASC);
	}

	ASC->GetGameplayAttributeValueChangeDelegate(UPDAttributeSet::GetStaminaAttribute())
		.AddUObject(this, &APDPlayerCharacter::OnStaminaChanged);

	ApplyGE(StaminaRegenEffectClass);
	ApplyGE(StaminaRegenBonusEffectClass);

	HungerDecayHandle  = ApplyGE(HungerDecayEffectClass);
	ThirstDecayHandle  = ApplyGE(ThirstDecayEffectClass);
	GasMaskDecayHandle = ApplyGE(GasMaskDecayEffectClass);
}

void APDPlayerCharacter::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (!AttributeSet||!VisionComponent) return;
	const float MaxStamina=AttributeSet->GetMaxStamina();
	if (MaxStamina<=0.f) return;
	VisionComponent->UpdateStaminaScale(Data.NewValue/MaxStamina);
}

void APDPlayerCharacter::HandleDeath(AActor* Killer)
{
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

void APDPlayerCharacter::PickupWeapon(APDWeaponBase* Weapon)
{
	if (!Weapon) return;
	EWeaponSlot TargetSlot=GetSlotForWeaponType(Weapon->GetWeaponType());
	if (TargetSlot==EWeaponSlot::None) return;

	int32 Idx=static_cast<int32>(TargetSlot);
	// 슬롯이 차있을 때는 호출자(Interact/AutoEquip)에서 인벤토리 경로로 우회해야 한다.
	if (WeaponSlots[Idx]) return;

	WeaponSlots[Idx]=Weapon;
	Weapon->OnEquip(this);           
	Weapon->SetActorHiddenInGame(true);
	AttachActorToWeaponSocket(Weapon);
	OnWeaponPickedUp.Broadcast(Weapon);

	if (CurrentSlot==EWeaponSlot::None)
		SwitchToSlot(TargetSlot);
}

void APDPlayerCharacter::SwitchToSlot(EWeaponSlot Slot)
{
	if (Slot==CurrentSlot) return;

	if (QuickSlotComponent)
		QuickSlotComponent->CancelConsumableUse();

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
	if (ASC)
	{
		ASC->RemoveLooseGameplayTag(PDGameplayTags::Weapon_Type_Rifle);
		ASC->RemoveLooseGameplayTag(PDGameplayTags::Weapon_Type_Shotgun);
		ASC->RemoveLooseGameplayTag(PDGameplayTags::Weapon_Type_Sniper);
		ASC->RemoveLooseGameplayTag(PDGameplayTags::Weapon_Type_Melee);
		ASC->AddLooseGameplayTag(NewWeapon->GetWeaponTypeTag());
	}
	NewWeapon->SetActorHiddenInGame(false);
	AttachActorToWeaponSocket(NewWeapon);

	// AnimInstance에 장착 통보 — 태그 추가 이후에 호출해야 RaiseMontage 태그 조회 가능
	if (AnimInst)
		AnimInst->OnWeaponEquipped(Cast<APDRangedWeaponBase>(NewWeapon));

	OnWeaponSwapped.Broadcast(NewWeapon, Slot);
}

void APDPlayerCharacter::DropCurrentWeapon()
{
	APDWeaponBase* CurWeapon=GetCurrentWeapon();
	if (!CurWeapon) return;

	if (UPDAnimInstance* AnimInst = GetMesh()
		? Cast<UPDAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr)
	{
		AnimInst->OnWeaponUnequipped(Cast<APDRangedWeaponBase>(CurWeapon));
	}
	CurWeapon->OnUnequip();
	CurWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	// 떨어뜨린 무기를 다시 픽업할 수 있도록 PickupCollision 복구 + 시뮬레이션 ON.
	CurWeapon->SetDropped(true);
	WeaponSlots[static_cast<int32>(CurrentSlot)]=nullptr;
	CurrentSlot=EWeaponSlot::None;
	ASC->RemoveLooseGameplayTag(CurWeapon->GetWeaponTypeTag());
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
	FPDInventorySlot TempSlot;
	TempSlot.ItemData = ItemData;
	TempSlot.Quantity = 1;
	TempSlot.bIsEmpty = false;
	TempSlot.ModificationLevel = 0;
	return TryAutoEquipWeaponSlot(TempSlot);
}

bool APDPlayerCharacter::TryAutoEquipWeaponSlot(const FPDInventorySlot& ItemSlot)
{
	const FPDItemData& ItemData = ItemSlot.ItemData;
	if (!ItemData.WeaponClass) return false;

	const EWeaponSlot TargetSlot = GetSlotForWeaponType(ItemData.WeaponType);
	if (TargetSlot == EWeaponSlot::None) return false;

	// 이미 해당 슬롯에 무기가 있으면 자동 장착 안 함(호출자가 인벤토리로 보내야 함).
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

	// 기존 무기/GAS 레벨 시스템은 1부터 시작하므로 +0 개조는 Lv.1로 매핑한다.
	SpawnedWeapon->SetLevel(FMath::Max(1, ItemSlot.ModificationLevel + 1));
	PickupWeapon(SpawnedWeapon);
	return true;
}


bool APDPlayerCharacter::RemoveEquippedWeaponItem(const FPDItemData& ItemData, bool bDestroyWeaponActor)
{
	const EWeaponSlot TargetSlot = GetSlotForWeaponType(ItemData.WeaponType);
	if (TargetSlot == EWeaponSlot::None)
	{
		return false;
	}

	const int32 SlotIndex = static_cast<int32>(TargetSlot);
	if (!WeaponSlots.IsValidIndex(SlotIndex) || !WeaponSlots[SlotIndex])
	{
		return false;
	}

	APDWeaponBase* WeaponToRemove = WeaponSlots[SlotIndex];
	WeaponToRemove->OnUnequip();
	WeaponToRemove->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	WeaponToRemove->SetActorHiddenInGame(true);

	if (ASC)
	{
		ASC->RemoveLooseGameplayTag(WeaponToRemove->GetWeaponTypeTag());
	}

	WeaponSlots[SlotIndex] = nullptr;
	if (CurrentSlot == TargetSlot)
	{
		CurrentSlot = EWeaponSlot::None;
	}

	if (bDestroyWeaponActor)
	{
		WeaponToRemove->Destroy();
	}

	return true;
}

void APDPlayerCharacter::ResetToBase()
{
	if (!ASC || !AttributeSet) return;
	
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

	auto ApplyGE = [&](TSubclassOf<UGameplayEffect> GEClass) -> FActiveGameplayEffectHandle
	{
		if (!GEClass) return FActiveGameplayEffectHandle();
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GEClass, 1.f, Context);
		if (Spec.IsValid())
			return ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		return FActiveGameplayEffectHandle();
	};
	
	HungerDecayHandle=ApplyGE(HungerDecayEffectClass);
	ThirstDecayHandle=ApplyGE(ThirstDecayEffectClass);
	GasMaskDecayHandle=ApplyGE(GasMaskDecayEffectClass);
}

void APDPlayerCharacter::TryInteract()
{
	if (QuickSlotComponent)
		QuickSlotComponent->CancelConsumableUse();

	if (InteractionComponent)
	{
		InteractionComponent->Interact();
	}
}

void APDPlayerCharacter::OnWeaponTypeTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	UE_LOG(LogTemp, Warning, TEXT("[Layer] TagChanged: %s | Count: %d"), *Tag.ToString(), NewCount);
	if (NewCount==0) return;

	APDWeaponBase* CurWeapon=GetCurrentWeapon();
	if (!IsValid(CurWeapon))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Layer] CurWeapon NULL"));
		return;
	}

	TSubclassOf<UAnimInstance> LayerClass=CurWeapon->GetWeaponAnimLayerClass();
	UE_LOG(LogTemp, Warning, TEXT("[Layer] LayerClass: %s"), LayerClass ? *LayerClass->GetName() : TEXT("NULL"));

	if (!LayerClass) { LinkDefaultAnimLayer(); return; }
	if (USkeletalMeshComponent* SkelMesh=GetMesh())
		SkelMesh->LinkAnimClassLayers(LayerClass);
}

void APDPlayerCharacter::LinkDefaultAnimLayer()
{
	if (!DefaultAnimLayerClass) return;
	if (USkeletalMeshComponent* SkelMesh=GetMesh())
		SkelMesh->LinkAnimClassLayers(DefaultAnimLayerClass);
}
