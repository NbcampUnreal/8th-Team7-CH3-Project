#include "Items/PDEquipmentModificationComponent.h"

#include "Items/PDInventoryComponent.h"

UPDEquipmentModificationComponent::UPDEquipmentModificationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UPDEquipmentModificationComponent::ConvertModificationLevelToGasLevel(int32 ModificationLevel) const
{
	return FMath::Max(1, ModificationLevel + 1);
}

float UPDEquipmentModificationComponent::GetCurveValue(FName RowName, int32 TargetLevel, float DefaultValue) const
{
	if (!ModificationCurveTable || RowName.IsNone())
	{
		return DefaultValue;
	}

	static const FString Context(TEXT("EquipmentModification"));
	if (const FRealCurve* Curve = ModificationCurveTable->FindCurve(RowName, Context, false))
	{
		return Curve->Eval(static_cast<float>(TargetLevel), DefaultValue);
	}

	return DefaultValue;
}


int32 UPDEquipmentModificationComponent::GetGoldCostFromRecipe(int32 TargetLevel) const
{
	if (!ModificationRecipeDataTable)
	{
		return 0;
	}

	TArray<FPDModificationRecipeData*> Rows;
	ModificationRecipeDataTable->GetAllRows<FPDModificationRecipeData>(TEXT("EquipmentModificationRecipeGoldCost"), Rows);

	for (const FPDModificationRecipeData* Row : Rows)
	{
		if (Row && Row->TargetLevel == TargetLevel)
		{
			return FMath::Max(0, Row->GoldCost);
		}
	}

	return 0;
}

float UPDEquipmentModificationComponent::NormalizeSuccessRate(float RawRate) const
{
	if (RawRate > 1.f)
	{
		RawRate *= 0.01f;
	}

	return FMath::Clamp(RawRate, 0.f, 1.f);
}

TArray<FPDModificationMaterialRequirement> UPDEquipmentModificationComponent::GetRequiredMaterials(int32 TargetLevel) const
{
	TArray<FPDModificationMaterialRequirement> Result;

	if (!ModificationRecipeDataTable)
	{
		return Result;
	}

	TArray<FPDModificationRecipeData*> Rows;
	ModificationRecipeDataTable->GetAllRows<FPDModificationRecipeData>(TEXT("EquipmentModificationRecipe"), Rows);

	for (const FPDModificationRecipeData* Row : Rows)
	{
		if (!Row || Row->TargetLevel != TargetLevel || Row->RequiredItemID.IsNone() || Row->Quantity <= 0)
		{
			continue;
		}

		FPDModificationMaterialRequirement& Entry = Result.AddDefaulted_GetRef();
		Entry.RequiredItemID = Row->RequiredItemID;
		Entry.Quantity = Row->Quantity;
	}

	return Result;
}

bool UPDEquipmentModificationComponent::GetBoostMaterial(EPDModificationBoostType BoostType, FName& OutItemID, int32& OutQuantity) const
{
	OutItemID = NAME_None;
	OutQuantity = 0;

	if (BoostType == EPDModificationBoostType::None || !ModificationBoostDataTable)
	{
		return false;
	}

	TArray<FPDModificationBoostData*> Rows;
	ModificationBoostDataTable->GetAllRows<FPDModificationBoostData>(TEXT("EquipmentModificationBoost"), Rows);

	for (const FPDModificationBoostData* Row : Rows)
	{
		if (!Row || Row->BoostType != BoostType || Row->BoostItemID.IsNone() || Row->Quantity <= 0)
		{
			continue;
		}

		OutItemID = Row->BoostItemID;
		OutQuantity = FMath::Max(0, Row->Quantity);
		return true;
	}

	return false;
}

float UPDEquipmentModificationComponent::GetBoostRate(EPDModificationBoostType BoostType, int32 TargetLevel) const
{
	switch (BoostType)
	{
	case EPDModificationBoostType::Low:
		return NormalizeSuccessRate(GetCurveValue(LowBoostRateCurveRowName, TargetLevel, 0.f));
	case EPDModificationBoostType::Mid:
		return NormalizeSuccessRate(GetCurveValue(MidBoostRateCurveRowName, TargetLevel, 0.f));
	case EPDModificationBoostType::High:
		return NormalizeSuccessRate(GetCurveValue(HighBoostRateCurveRowName, TargetLevel, 0.f));
	default:
		return 0.f;
	}
}

void UPDEquipmentModificationComponent::FillApplicableBonus(const FPDInventorySlot& TargetSlot, int32 TargetLevel, FPDModificationPreview& OutPreview) const
{
	OutPreview.AttackBonus = 0.f;
	OutPreview.DefenseBonus = 0.f;

	const FPDItemData& ItemData = TargetSlot.ItemData;
	if (ItemData.ItemType != EPDItemType::Equipment)
	{
		return;
	}

	if (ItemData.EquipmentSlotType == EPDEquipmentSlotType::Weapon || ItemData.WeaponType != EWeaponType::None)
	{
		OutPreview.AttackBonus = GetCurveValue(AttackBonusCurveRowName, TargetLevel, 0.f);
		return;
	}

	if (ItemData.EquipmentSlotType == EPDEquipmentSlotType::Head || ItemData.EquipmentSlotType == EPDEquipmentSlotType::Armor)
	{
		OutPreview.DefenseBonus = GetCurveValue(DefenseBonusCurveRowName, TargetLevel, 0.f);
	}
}

bool UPDEquipmentModificationComponent::GetModificationPreview(const FPDInventorySlot& TargetSlot, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult) const
{
	return GetModificationPreviewWithBoost(TargetSlot, EPDModificationBoostType::None, OutPreview, OutResult);
}

bool UPDEquipmentModificationComponent::GetModificationPreviewWithBoost(const FPDInventorySlot& TargetSlot, EPDModificationBoostType BoostType, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult) const
{
	OutPreview = FPDModificationPreview();

	if (TargetSlot.IsEmpty())
	{
		OutResult = EPDModificationResult::InvalidSlot;
		return false;
	}

	if (TargetSlot.ItemData.ItemType != EPDItemType::Equipment)
	{
		OutResult = EPDModificationResult::NotEquipment;
		return false;
	}

	const int32 CurrentLevel = FMath::Max(0, TargetSlot.ModificationLevel);
	if (CurrentLevel >= MaxModificationLevel)
	{
		OutPreview.CurrentModificationLevel = CurrentLevel;
		OutPreview.TargetModificationLevel = CurrentLevel;
		OutPreview.TargetGasLevel = ConvertModificationLevelToGasLevel(CurrentLevel);
		OutResult = EPDModificationResult::AlreadyMaxLevel;
		return false;
	}

	const int32 TargetLevel = CurrentLevel + 1;
	OutPreview.CurrentModificationLevel = CurrentLevel;
	OutPreview.TargetModificationLevel = TargetLevel;
	OutPreview.TargetGasLevel = ConvertModificationLevelToGasLevel(TargetLevel);
	OutPreview.GoldCost = GetGoldCostFromRecipe(TargetLevel);
	OutPreview.BaseSuccessRate = NormalizeSuccessRate(GetCurveValue(SuccessRateCurveRowName, TargetLevel, 1.f));
	OutPreview.SelectedBoostType = BoostType;
	OutPreview.BoostSuccessRate = GetBoostRate(BoostType, TargetLevel);
	OutPreview.SuccessRate = FMath::Clamp(OutPreview.BaseSuccessRate + OutPreview.BoostSuccessRate, 0.f, 1.f);
	FillApplicableBonus(TargetSlot, TargetLevel, OutPreview);
	OutPreview.RequiredMaterials = GetRequiredMaterials(TargetLevel);

	FName BoostItemID;
	int32 BoostItemQuantity = 0;
	if (GetBoostMaterial(BoostType, BoostItemID, BoostItemQuantity))
	{
		OutPreview.BoostItemID = BoostItemID;
		OutPreview.BoostItemQuantity = BoostItemQuantity;
	}

	if (!ModificationCurveTable)
	{
		OutResult = EPDModificationResult::MissingCurveTable;
		return false;
	}

	OutResult = EPDModificationResult::Success;
	return true;
}

bool UPDEquipmentModificationComponent::HasRequiredMaterials(const UPDInventoryComponent* InventoryComponent, const TArray<FPDModificationMaterialRequirement>& Materials) const
{
	if (!InventoryComponent)
	{
		return false;
	}

	for (const FPDModificationMaterialRequirement& Material : Materials)
	{
		if (!InventoryComponent->HasItem(Material.RequiredItemID, Material.Quantity))
		{
			return false;
		}
	}

	return true;
}


bool UPDEquipmentModificationComponent::HasRequiredMaterialSlot(const UPDInventoryComponent* InventoryComponent, int32 MaterialSlotIndex, const TArray<FPDModificationMaterialRequirement>& Materials) const
{
	if (!InventoryComponent)
	{
		return false;
	}

	if (Materials.Num() <= 0)
	{
		return true;
	}

	if (!InventoryComponent->Items.IsValidIndex(MaterialSlotIndex))
	{
		return false;
	}

	const FPDInventorySlot& MaterialSlot = InventoryComponent->Items[MaterialSlotIndex];
	if (MaterialSlot.IsEmpty())
	{
		return false;
	}

	for (const FPDModificationMaterialRequirement& Material : Materials)
	{
		if (MaterialSlot.ItemData.ItemID == Material.RequiredItemID && MaterialSlot.Quantity >= Material.Quantity)
		{
			return true;
		}
	}

	return false;
}

bool UPDEquipmentModificationComponent::ConsumeRequiredMaterials(UPDInventoryComponent* InventoryComponent, const TArray<FPDModificationMaterialRequirement>& Materials) const
{
	if (!InventoryComponent)
	{
		return false;
	}

	for (const FPDModificationMaterialRequirement& Material : Materials)
	{
		if (!InventoryComponent->RemoveItem(Material.RequiredItemID, Material.Quantity))
		{
			return false;
		}
	}

	return true;
}


bool UPDEquipmentModificationComponent::ConsumeRequiredMaterialSlot(UPDInventoryComponent* InventoryComponent, int32 MaterialSlotIndex, const TArray<FPDModificationMaterialRequirement>& Materials) const
{
	if (!HasRequiredMaterialSlot(InventoryComponent, MaterialSlotIndex, Materials))
	{
		return false;
	}

	if (Materials.Num() <= 0)
	{
		return true;
	}

	const FPDInventorySlot& MaterialSlot = InventoryComponent->Items[MaterialSlotIndex];
	for (const FPDModificationMaterialRequirement& Material : Materials)
	{
		if (MaterialSlot.ItemData.ItemID == Material.RequiredItemID && MaterialSlot.Quantity >= Material.Quantity)
		{
			return InventoryComponent->RemoveItemFromSlot(MaterialSlotIndex, Material.Quantity);
		}
	}

	return false;
}

bool UPDEquipmentModificationComponent::CanModifyInventorySlot(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult) const
{
	return CanModifyInventorySlotWithBoost(InventoryComponent, InventorySlotIndex, EPDModificationBoostType::None, OutPreview, OutResult);
}

bool UPDEquipmentModificationComponent::CanModifyInventorySlotWithBoost(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, EPDModificationBoostType BoostType, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult) const
{
	if (!InventoryComponent)
	{
		OutResult = EPDModificationResult::InvalidInventory;
		return false;
	}

	if (!InventoryComponent->Items.IsValidIndex(InventorySlotIndex))
	{
		OutResult = EPDModificationResult::InvalidSlot;
		return false;
	}

	if (!GetModificationPreviewWithBoost(InventoryComponent->Items[InventorySlotIndex], BoostType, OutPreview, OutResult))
	{
		return false;
	}

	if (InventoryComponent->GetGold() < OutPreview.GoldCost)
	{
		OutResult = EPDModificationResult::NotEnoughGold;
		return false;
	}

	if (!HasRequiredMaterials(InventoryComponent, OutPreview.RequiredMaterials))
	{
		OutResult = EPDModificationResult::NotEnoughMaterials;
		return false;
	}

	if (!OutPreview.BoostItemID.IsNone() && OutPreview.BoostItemQuantity > 0 && !InventoryComponent->HasItem(OutPreview.BoostItemID, OutPreview.BoostItemQuantity))
	{
		OutResult = EPDModificationResult::NotEnoughMaterials;
		return false;
	}

	OutResult = EPDModificationResult::Success;
	return true;
}


bool UPDEquipmentModificationComponent::CanModifyInventorySlotWithMaterialSlot(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, int32 MaterialSlotIndex, EPDModificationBoostType BoostType, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult) const
{
	if (!CanModifyInventorySlotWithBoost(InventoryComponent, InventorySlotIndex, BoostType, OutPreview, OutResult))
	{
		return false;
	}

	if (!HasRequiredMaterialSlot(InventoryComponent, MaterialSlotIndex, OutPreview.RequiredMaterials))
	{
		OutResult = EPDModificationResult::NotEnoughMaterials;
		return false;
	}

	OutResult = EPDModificationResult::Success;
	return true;
}

bool UPDEquipmentModificationComponent::TryModifyInventorySlot(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult)
{
	return TryModifyInventorySlotWithBoost(InventoryComponent, InventorySlotIndex, EPDModificationBoostType::None, OutPreview, OutResult);
}

bool UPDEquipmentModificationComponent::TryModifyInventorySlotWithBoost(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, EPDModificationBoostType BoostType, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult)
{
	if (!CanModifyInventorySlotWithBoost(InventoryComponent, InventorySlotIndex, BoostType, OutPreview, OutResult))
	{
		OnModificationFinished.Broadcast(InventorySlotIndex, 0, false, OutResult);
		return false;
	}

	if (!InventoryComponent->SpendGold(OutPreview.GoldCost))
	{
		OutResult = EPDModificationResult::NotEnoughGold;
		OnModificationFinished.Broadcast(InventorySlotIndex, InventoryComponent->Items[InventorySlotIndex].ModificationLevel, false, OutResult);
		return false;
	}

	if (!ConsumeRequiredMaterials(InventoryComponent, OutPreview.RequiredMaterials))
	{
		OutResult = EPDModificationResult::NotEnoughMaterials;
		OnModificationFinished.Broadcast(InventorySlotIndex, InventoryComponent->Items[InventorySlotIndex].ModificationLevel, false, OutResult);
		return false;
	}

	if (!OutPreview.BoostItemID.IsNone() && OutPreview.BoostItemQuantity > 0 && !InventoryComponent->RemoveItem(OutPreview.BoostItemID, OutPreview.BoostItemQuantity))
	{
		OutResult = EPDModificationResult::NotEnoughMaterials;
		OnModificationFinished.Broadcast(InventorySlotIndex, InventoryComponent->Items[InventorySlotIndex].ModificationLevel, false, OutResult);
		return false;
	}

	const bool bSuccess = FMath::FRand() <= OutPreview.SuccessRate;
	FPDInventorySlot& Slot = InventoryComponent->Items[InventorySlotIndex];

	if (bSuccess)
	{
		Slot.ModificationLevel = OutPreview.TargetModificationLevel;
		OutResult = EPDModificationResult::Success;
	}
	else
	{
		OutResult = EPDModificationResult::FailedByChance;
	}

	InventoryComponent->OnInventoryChanged.Broadcast();
	OnModificationFinished.Broadcast(InventorySlotIndex, Slot.ModificationLevel, bSuccess, OutResult);
	return bSuccess;
}

bool UPDEquipmentModificationComponent::TryModifyInventorySlotWithMaterialSlot(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, int32 MaterialSlotIndex, EPDModificationBoostType BoostType, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult)
{
	if (!CanModifyInventorySlotWithMaterialSlot(InventoryComponent, InventorySlotIndex, MaterialSlotIndex, BoostType, OutPreview, OutResult))
	{
		OnModificationFinished.Broadcast(InventorySlotIndex, 0, false, OutResult);
		return false;
	}

	if (!InventoryComponent->SpendGold(OutPreview.GoldCost))
	{
		OutResult = EPDModificationResult::NotEnoughGold;
		OnModificationFinished.Broadcast(InventorySlotIndex, InventoryComponent->Items[InventorySlotIndex].ModificationLevel, false, OutResult);
		return false;
	}

	if (!ConsumeRequiredMaterialSlot(InventoryComponent, MaterialSlotIndex, OutPreview.RequiredMaterials))
	{
		OutResult = EPDModificationResult::NotEnoughMaterials;
		OnModificationFinished.Broadcast(InventorySlotIndex, InventoryComponent->Items[InventorySlotIndex].ModificationLevel, false, OutResult);
		return false;
	}

	if (!OutPreview.BoostItemID.IsNone() && OutPreview.BoostItemQuantity > 0 && !InventoryComponent->RemoveItem(OutPreview.BoostItemID, OutPreview.BoostItemQuantity))
	{
		OutResult = EPDModificationResult::NotEnoughMaterials;
		OnModificationFinished.Broadcast(InventorySlotIndex, InventoryComponent->Items[InventorySlotIndex].ModificationLevel, false, OutResult);
		return false;
	}

	const bool bSuccess = FMath::FRand() <= OutPreview.SuccessRate;
	FPDInventorySlot& Slot = InventoryComponent->Items[InventorySlotIndex];

	if (bSuccess)
	{
		Slot.ModificationLevel = OutPreview.TargetModificationLevel;
		OutResult = EPDModificationResult::Success;
	}
	else
	{
		OutResult = EPDModificationResult::FailedByChance;
	}

	InventoryComponent->OnInventoryChanged.Broadcast();
	OnModificationFinished.Broadcast(InventorySlotIndex, Slot.ModificationLevel, bSuccess, OutResult);
	return bSuccess;
}
