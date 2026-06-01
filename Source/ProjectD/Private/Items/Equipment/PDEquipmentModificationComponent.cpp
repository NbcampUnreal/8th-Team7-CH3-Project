#include "Items/Equipment/PDEquipmentModificationComponent.h"

#include "Engine/DataTable.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectIterator.h"
#include "Items/Containers/PDInventoryComponent.h"

UPDEquipmentModificationComponent::UPDEquipmentModificationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

int32 UPDEquipmentModificationComponent::ConvertModificationLevelToGasLevel(int32 ModificationLevel) const
{
	return FMath::Max(1, ModificationLevel + 1);
}

// === 2ë²?RPC êµ¬ì¡° ?µí•© (Source??ë¡œì§ ?„ì— ë©€?°í”Œ?ˆì´ RPC ?ˆì´??ì¶”ê?) ===
void UPDEquipmentModificationComponent::ServerTryModifyInventorySlotWithBoost_Implementation(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, EPDModificationBoostType BoostType)
{
	FPDModificationPreview Preview;
	EPDModificationResult Result = EPDModificationResult::InvalidInventory;
	TryModifyInventorySlotWithBoost(InventoryComponent, InventorySlotIndex, BoostType, Preview, Result);
}

void UPDEquipmentModificationComponent::ClientModificationFinished_Implementation(int32 InventorySlotIndex, int32 NewModificationLevel, bool bSuccess, EPDModificationResult Result)
{
	OnModificationFinished.Broadcast(InventorySlotIndex, NewModificationLevel, bSuccess, Result);
}

void UPDEquipmentModificationComponent::BroadcastModificationFinished(int32 InventorySlotIndex, int32 NewModificationLevel, bool bSuccess, EPDModificationResult Result)
{
	// ?œë²„ ì¸??™ìž‘ ???´ë¼?´ì–¸?¸ì—ê²?ê²°ê³¼ ?„íŒŒ.
	OnModificationFinished.Broadcast(InventorySlotIndex, NewModificationLevel, bSuccess, Result);
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ClientModificationFinished(InventorySlotIndex, NewModificationLevel, bSuccess, Result);
	}
}

namespace
{
	const FName UpgradeMaterialItemID(TEXT("IT_MISC_UpgradeMaterial"));
	const FName LowBoostItemID(TEXT("IT_MISC_Upgrade_Material_1"));
	const FName MidBoostItemID(TEXT("IT_MISC_Upgrade_Material_2"));
	const FName HighBoostItemID(TEXT("IT_MISC_Upgrade_Material_3"));

	int32 ParsePositiveIntegerName(FName RowName)
	{
		const FString RowString = RowName.ToString();
		if (RowString.IsNumeric())
		{
			return FMath::Max(0, FCString::Atoi(*RowString));
		}

		return 0;
	}

	int32 ResolveRecipeTargetLevel(FName RowName, const FPDModificationRecipeData* Row)
	{
		const int32 RowNameLevel = ParsePositiveIntegerName(RowName);
		if (RowNameLevel > 0)
		{
			return RowNameLevel;
		}

		return Row ? FMath::Max(0, Row->TargetLevel) : 0;
	}

	void AddFallbackBoostItem(FName ItemID, EPDModificationBoostType BoostType, TArray<FPDModificationBoostData>& Items)
	{
		FPDModificationBoostData& Entry = Items.AddDefaulted_GetRef();
		Entry.BoostItemID = ItemID;
		Entry.BoostType = BoostType;
		Entry.Quantity = 1;
	}
}

const UDataTable* UPDEquipmentModificationComponent::ResolveDataTable(const UDataTable* AssignedTable, const TCHAR* AssetName) const
{
	if (AssignedTable || !AssetName || !AssetName[0])
	{
		return AssignedTable;
	}

	const FName TargetAssetName(AssetName);
	for (TObjectIterator<UDataTable> It; It; ++It)
	{
		const UDataTable* LoadedTable = *It;
		if (LoadedTable && LoadedTable->GetFName() == TargetAssetName)
		{
			return LoadedTable;
		}
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByClass(UDataTable::StaticClass()->GetClassPathName(), AssetDataList, true);
	for (const FAssetData& AssetData : AssetDataList)
	{
		if (AssetData.AssetName == TargetAssetName)
		{
			if (UDataTable* Table = Cast<UDataTable>(AssetData.GetAsset()))
			{
				return Table;
			}
		}
	}

	const TArray<FString> Paths =
	{
		FString::Printf(TEXT("/Game/%s.%s"), AssetName, AssetName),
		FString::Printf(TEXT("/Game/Data/%s.%s"), AssetName, AssetName),
		FString::Printf(TEXT("/Game/DataTable/%s.%s"), AssetName, AssetName),
		FString::Printf(TEXT("/Game/DataTables/%s.%s"), AssetName, AssetName),
		FString::Printf(TEXT("/Game/Blueprints/Data/%s.%s"), AssetName, AssetName),
		FString::Printf(TEXT("/Game/Blueprints/DataTable/%s.%s"), AssetName, AssetName),
		FString::Printf(TEXT("/Game/Blueprints/DataTables/%s.%s"), AssetName, AssetName),
		FString::Printf(TEXT("/Game/Blueprints/Data/Item/%s.%s"), AssetName, AssetName),
		FString::Printf(TEXT("/Game/Blueprints/Data/Items/%s.%s"), AssetName, AssetName),
		FString::Printf(TEXT("/Game/Main/Blueprints/Data/%s.%s"), AssetName, AssetName),
		FString::Printf(TEXT("/Game/Main/Blueprints/DataTable/%s.%s"), AssetName, AssetName),
		FString::Printf(TEXT("/Game/Main/Blueprints/DataTables/%s.%s"), AssetName, AssetName)
	};

	for (const FString& Path : Paths)
	{
		if (UDataTable* Table = LoadObject<UDataTable>(nullptr, *Path))
		{
			return Table;
		}
	}

	return nullptr;
}

const UCurveTable* UPDEquipmentModificationComponent::ResolveModificationCurveTable() const
{
	if (ModificationCurveTable)
	{
		return ModificationCurveTable;
	}

	static const FName TargetAssetName(TEXT("CT_ModificationStats"));
	for (TObjectIterator<UCurveTable> It; It; ++It)
	{
		const UCurveTable* LoadedTable = *It;
		if (LoadedTable && LoadedTable->GetFName() == TargetAssetName)
		{
			return LoadedTable;
		}
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByClass(UCurveTable::StaticClass()->GetClassPathName(), AssetDataList, true);
	for (const FAssetData& AssetData : AssetDataList)
	{
		if (AssetData.AssetName == TargetAssetName)
		{
			if (UCurveTable* Table = Cast<UCurveTable>(AssetData.GetAsset()))
			{
				return Table;
			}
		}
	}

	return nullptr;
}

const UDataTable* UPDEquipmentModificationComponent::ResolveRecipeDataTable() const
{
	return ResolveDataTable(ModificationRecipeDataTable, TEXT("DT_ModificationRecipeData"));
}

const UDataTable* UPDEquipmentModificationComponent::ResolveBoostDataTable() const
{
	return ResolveDataTable(ModificationBoostDataTable, TEXT("DT_ModificationBoostData"));
}

float UPDEquipmentModificationComponent::GetCurveValue(FName RowName, int32 TargetLevel, float DefaultValue) const
{
	const UCurveTable* CurveTable = ResolveModificationCurveTable();
	if (!CurveTable || RowName.IsNone())
	{
		return DefaultValue;
	}

	static const FString Context(TEXT("EquipmentModification"));
	if (const FRealCurve* Curve = CurveTable->FindCurve(RowName, Context, false))
	{
		return Curve->Eval(static_cast<float>(TargetLevel), DefaultValue);
	}

	return DefaultValue;
}

int32 UPDEquipmentModificationComponent::GetResolvedMaxModificationLevel() const
{
	const UCurveTable* CurveTable = ResolveModificationCurveTable();
	if (CurveTable && !SuccessRateCurveRowName.IsNone())
	{
		static const FString Context(TEXT("EquipmentModificationMaxLevel"));
		if (const FRealCurve* Curve = CurveTable->FindCurve(SuccessRateCurveRowName, Context, false))
		{
			float MinTime = 0.f;
			float MaxTime = 0.f;
			Curve->GetTimeRange(MinTime, MaxTime);
			return FMath::Max(1, FMath::FloorToInt(MaxTime));
		}
	}

	return FMath::Max(1, MaxModificationLevel);
}

int32 UPDEquipmentModificationComponent::GetGoldCostFromRecipe(int32 TargetLevel) const
{
	const UDataTable* RecipeTable = ResolveRecipeDataTable();
	if (!RecipeTable)
	{
		return 0;
	}

	int32 GoldCost = 0;
	bool bFound = false;
	for (const TPair<FName, uint8*>& Pair : RecipeTable->GetRowMap())
	{
		const FPDModificationRecipeData* Row = reinterpret_cast<const FPDModificationRecipeData*>(Pair.Value);
		if (!Row || ResolveRecipeTargetLevel(Pair.Key, Row) != TargetLevel)
		{
			continue;
		}

		GoldCost = FMath::Max(GoldCost, Row->GoldCost);
		bFound = true;
	}

	return bFound ? FMath::Max(0, GoldCost) : 0;
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
	TMap<FName, int32> QuantityByItemID;
	const UDataTable* RecipeTable = ResolveRecipeDataTable();
	if (RecipeTable)
	{
		for (const TPair<FName, uint8*>& Pair : RecipeTable->GetRowMap())
		{
			const FPDModificationRecipeData* Row = reinterpret_cast<const FPDModificationRecipeData*>(Pair.Value);
			if (!Row || ResolveRecipeTargetLevel(Pair.Key, Row) != TargetLevel || Row->RequiredItemID.IsNone() || Row->Quantity <= 0)
			{
				continue;
			}

			QuantityByItemID.FindOrAdd(Row->RequiredItemID) += Row->Quantity;
		}
	}

	TArray<FPDModificationMaterialRequirement> Result;
	for (const TPair<FName, int32>& Pair : QuantityByItemID)
	{
		FPDModificationMaterialRequirement& Entry = Result.AddDefaulted_GetRef();
		Entry.RequiredItemID = Pair.Key;
		Entry.Quantity = Pair.Value;
	}

	return Result;
}

bool UPDEquipmentModificationComponent::GetBoostMaterial(EPDModificationBoostType BoostType, FName& OutItemID, int32& OutQuantity) const
{
	OutItemID = NAME_None;
	OutQuantity = 0;

	if (BoostType == EPDModificationBoostType::None)
	{
		return false;
	}

	const UDataTable* BoostTable = ResolveBoostDataTable();
	if (BoostTable)
	{
		for (const TPair<FName, uint8*>& Pair : BoostTable->GetRowMap())
		{
			const FPDModificationBoostData* Row = reinterpret_cast<const FPDModificationBoostData*>(Pair.Value);
			if (!Row || Row->BoostType != BoostType || Row->BoostItemID.IsNone() || Row->Quantity <= 0)
			{
				continue;
			}

			OutItemID = Row->BoostItemID;
			OutQuantity = Row->Quantity;
			return true;
		}
	}

	switch (BoostType)
	{
	case EPDModificationBoostType::Low:
		OutItemID = LowBoostItemID;
		OutQuantity = 1;
		return true;
	case EPDModificationBoostType::Mid:
		OutItemID = MidBoostItemID;
		OutQuantity = 1;
		return true;
	case EPDModificationBoostType::High:
		OutItemID = HighBoostItemID;
		OutQuantity = 1;
		return true;
	default:
		return false;
	}
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

bool UPDEquipmentModificationComponent::IsRegisteredBoostItemID(FName ItemID) const
{
	EPDModificationBoostType BoostType = EPDModificationBoostType::None;
	int32 Quantity = 0;
	return GetBoostMaterialByItemID(ItemID, BoostType, Quantity);
}

bool UPDEquipmentModificationComponent::IsModificationMaterialItemID(FName ItemID) const
{
	if (ItemID.IsNone())
	{
		return false;
	}

	const UDataTable* RecipeTable = ResolveRecipeDataTable();
	if (RecipeTable)
	{
		for (const TPair<FName, uint8*>& Pair : RecipeTable->GetRowMap())
		{
			const FPDModificationRecipeData* Row = reinterpret_cast<const FPDModificationRecipeData*>(Pair.Value);
			if (Row && Row->RequiredItemID == ItemID && Row->Quantity > 0)
			{
				return true;
			}
		}
	}

	return ItemID == UpgradeMaterialItemID;
}

bool UPDEquipmentModificationComponent::IsModifiableEquipmentSlot(const FPDInventorySlot& TargetSlot) const
{
	if (TargetSlot.IsEmpty() || TargetSlot.ItemData.ItemType != EPDItemType::Equipment)
	{
		return false;
	}

	const FPDItemData& ItemData = TargetSlot.ItemData;
	return ItemData.EquipmentSlotType == EPDEquipmentSlotType::Weapon || ItemData.EquipmentSlotType == EPDEquipmentSlotType::Armor || ItemData.EquipmentSlotType == EPDEquipmentSlotType::Head || ItemData.WeaponType != EWeaponType::None;
}

bool UPDEquipmentModificationComponent::IsUpgradeMaterialSlot(const FPDInventorySlot& MaterialSlot) const
{
	return !MaterialSlot.IsEmpty() && IsModificationMaterialItemID(MaterialSlot.ItemData.ItemID);
}

bool UPDEquipmentModificationComponent::TryInferBoostMaterialByItemID(FName ItemID, EPDModificationBoostType& OutBoostType, int32& OutQuantity) const
{
	OutBoostType = EPDModificationBoostType::None;
	OutQuantity = 0;

	if (ItemID == LowBoostItemID)
	{
		OutBoostType = EPDModificationBoostType::Low;
		OutQuantity = 1;
		return true;
	}

	if (ItemID == MidBoostItemID)
	{
		OutBoostType = EPDModificationBoostType::Mid;
		OutQuantity = 1;
		return true;
	}

	if (ItemID == HighBoostItemID)
	{
		OutBoostType = EPDModificationBoostType::High;
		OutQuantity = 1;
		return true;
	}

	return false;
}

bool UPDEquipmentModificationComponent::GetBoostMaterialByItemID(FName ItemID, EPDModificationBoostType& OutBoostType, int32& OutQuantity) const
{
	OutBoostType = EPDModificationBoostType::None;
	OutQuantity = 0;

	if (ItemID.IsNone())
	{
		return false;
	}

	const UDataTable* BoostTable = ResolveBoostDataTable();
	if (BoostTable)
	{
		for (const TPair<FName, uint8*>& Pair : BoostTable->GetRowMap())
		{
			const FPDModificationBoostData* Row = reinterpret_cast<const FPDModificationBoostData*>(Pair.Value);
			if (!Row || Row->BoostType == EPDModificationBoostType::None || Row->BoostItemID != ItemID || Row->Quantity <= 0)
			{
				continue;
			}

			OutBoostType = Row->BoostType;
			OutQuantity = Row->Quantity;
			return true;
		}
	}

	return TryInferBoostMaterialByItemID(ItemID, OutBoostType, OutQuantity);
}

bool UPDEquipmentModificationComponent::IsValidBoostSlot(const UPDInventoryComponent* InventoryComponent, int32 BoostSlotIndex, EPDModificationBoostType& OutBoostType, FName& OutBoostItemID, int32& OutQuantity) const
{
	OutBoostType = EPDModificationBoostType::None;
	OutBoostItemID = NAME_None;
	OutQuantity = 0;

	if (BoostSlotIndex == INDEX_NONE)
	{
		return true;
	}

	if (!InventoryComponent || !InventoryComponent->Items.IsValidIndex(BoostSlotIndex))
	{
		return false;
	}

	const FPDInventorySlot& BoostSlot = InventoryComponent->Items[BoostSlotIndex];
	if (BoostSlot.IsEmpty())
	{
		return false;
	}

	int32 RequiredQuantity = 0;
	EPDModificationBoostType BoostType = EPDModificationBoostType::None;
	if (!GetBoostMaterialByItemID(BoostSlot.ItemData.ItemID, BoostType, RequiredQuantity) || BoostSlot.Quantity < RequiredQuantity)
	{
		return false;
	}

	OutBoostType = BoostType;
	OutBoostItemID = BoostSlot.ItemData.ItemID;
	OutQuantity = RequiredQuantity;
	return true;
}

void UPDEquipmentModificationComponent::FillApplicableBonus(const FPDInventorySlot& TargetSlot, int32 TargetLevel, FPDModificationPreview& OutPreview) const
{
	OutPreview.AttackBonus = 0.f;
	OutPreview.DefenseBonus = 0.f;

	const FPDItemData& ItemData = TargetSlot.ItemData;
	if (!IsModifiableEquipmentSlot(TargetSlot))
	{
		return;
	}

	if (ItemData.EquipmentSlotType == EPDEquipmentSlotType::Weapon || ItemData.WeaponType != EWeaponType::None)
	{
		OutPreview.AttackBonus = GetCurveValue(AttackBonusCurveRowName, TargetLevel, 0.f);
		return;
	}

	OutPreview.DefenseBonus = GetCurveValue(DefenseBonusCurveRowName, TargetLevel, 0.f);
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

	if (!IsModifiableEquipmentSlot(TargetSlot))
	{
		OutResult = EPDModificationResult::NotEquipment;
		return false;
	}

	const int32 CurrentLevel = FMath::Max(0, TargetSlot.ModificationLevel);
	const int32 MaxLevel = GetResolvedMaxModificationLevel();
	if (CurrentLevel >= MaxLevel)
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
	OutPreview.RequiredMaterials = GetRequiredMaterials(TargetLevel);
	FillApplicableBonus(TargetSlot, TargetLevel, OutPreview);

	FName BoostItemID = NAME_None;
	int32 BoostItemQuantity = 0;
	if (GetBoostMaterial(BoostType, BoostItemID, BoostItemQuantity))
	{
		OutPreview.BoostItemID = BoostItemID;
		OutPreview.BoostItemQuantity = BoostItemQuantity;
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
	if (!IsUpgradeMaterialSlot(MaterialSlot))
	{
		return false;
	}

	for (const FPDModificationMaterialRequirement& Material : Materials)
	{
		if (MaterialSlot.ItemData.ItemID == Material.RequiredItemID)
		{
			return InventoryComponent->HasItem(Material.RequiredItemID, Material.Quantity);
		}
	}

	return false;
}

bool UPDEquipmentModificationComponent::ConsumeRequiredMaterials(UPDInventoryComponent* InventoryComponent, const TArray<FPDModificationMaterialRequirement>& Materials) const
{
	if (!InventoryComponent || !HasRequiredMaterials(InventoryComponent, Materials))
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
		if (MaterialSlot.ItemData.ItemID == Material.RequiredItemID)
		{
			return InventoryComponent->RemoveItem(Material.RequiredItemID, Material.Quantity);
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

bool UPDEquipmentModificationComponent::CanModifyInventorySlotWithMaterialAndBoostSlots(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, int32 MaterialSlotIndex, int32 BoostSlotIndex, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult) const
{
	EPDModificationBoostType BoostType = EPDModificationBoostType::None;
	FName BoostItemID = NAME_None;
	int32 BoostQuantity = 0;
	if (BoostSlotIndex != INDEX_NONE && !IsValidBoostSlot(InventoryComponent, BoostSlotIndex, BoostType, BoostItemID, BoostQuantity))
	{
		OutResult = EPDModificationResult::NotEnoughMaterials;
		return false;
	}

	if (!CanModifyInventorySlotWithMaterialSlot(InventoryComponent, InventorySlotIndex, MaterialSlotIndex, BoostType, OutPreview, OutResult))
	{
		return false;
	}

	if (!BoostItemID.IsNone())
	{
		OutPreview.SelectedBoostType = BoostType;
		OutPreview.BoostItemID = BoostItemID;
		OutPreview.BoostItemQuantity = BoostQuantity;
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

bool UPDEquipmentModificationComponent::TryModifyInventorySlotWithMaterialAndBoostSlots(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, int32 MaterialSlotIndex, int32 BoostSlotIndex, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult)
{
	if (!CanModifyInventorySlotWithMaterialAndBoostSlots(InventoryComponent, InventorySlotIndex, MaterialSlotIndex, BoostSlotIndex, OutPreview, OutResult))
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

	if (!OutPreview.BoostItemID.IsNone() && OutPreview.BoostItemQuantity > 0 && !InventoryComponent->RemoveItemFromSlot(BoostSlotIndex, OutPreview.BoostItemQuantity))
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
