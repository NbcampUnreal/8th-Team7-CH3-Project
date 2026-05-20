#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/CurveTable.h"
#include "Type/Types.h"
#include "PDEquipmentModificationComponent.generated.h"

class UPDInventoryComponent;
class UDataTable;

UENUM(BlueprintType)
enum class EPDModificationResult : uint8
{
	Success UMETA(DisplayName = "Success"),
	FailedByChance UMETA(DisplayName = "Failed By Chance"),
	InvalidInventory UMETA(DisplayName = "Invalid Inventory"),
	InvalidSlot UMETA(DisplayName = "Invalid Slot"),
	NotEquipment UMETA(DisplayName = "Not Equipment"),
	AlreadyMaxLevel UMETA(DisplayName = "Already Max Level"),
	MissingCurveTable UMETA(DisplayName = "Missing Curve Table"),
	NotEnoughGold UMETA(DisplayName = "Not Enough Gold"),
	NotEnoughMaterials UMETA(DisplayName = "Not Enough Materials"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FPDOnEquipmentModificationFinished, int32, InventorySlotIndex, int32, NewModificationLevel, bool, bSuccess, EPDModificationResult, Result);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTD_API UPDEquipmentModificationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDEquipmentModificationComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Equipment Modification")
	TObjectPtr<UCurveTable> ModificationCurveTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Equipment Modification")
	TObjectPtr<UDataTable> ModificationRecipeDataTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Equipment Modification")
	TObjectPtr<UDataTable> ModificationBoostDataTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Equipment Modification", meta = (ClampMin = "1"))
	int32 MaxModificationLevel = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Equipment Modification|Curve Rows")
	FName SuccessRateCurveRowName = TEXT("SuccessRate");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Equipment Modification|Curve Rows")
	FName AttackBonusCurveRowName = TEXT("AttackBonus");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Equipment Modification|Curve Rows")
	FName DefenseBonusCurveRowName = TEXT("DefenseBonus");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Equipment Modification|Curve Rows")
	FName LowBoostRateCurveRowName = TEXT("BoostRate_Low");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Equipment Modification|Curve Rows")
	FName MidBoostRateCurveRowName = TEXT("BoostRate_Mid");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PD|Equipment Modification|Curve Rows")
	FName HighBoostRateCurveRowName = TEXT("BoostRate_High");

	UPROPERTY(BlueprintAssignable, Category = "PD|Equipment Modification")
	FPDOnEquipmentModificationFinished OnModificationFinished;

	UFUNCTION(BlueprintPure, Category = "PD|Equipment Modification")
	int32 ConvertModificationLevelToGasLevel(int32 ModificationLevel) const;

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	bool GetModificationPreview(const FPDInventorySlot& TargetSlot, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult) const;

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	bool GetModificationPreviewWithBoost(const FPDInventorySlot& TargetSlot, EPDModificationBoostType BoostType, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult) const;

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	bool CanModifyInventorySlot(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult) const;

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	bool CanModifyInventorySlotWithBoost(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, EPDModificationBoostType BoostType, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult) const;

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	bool CanModifyInventorySlotWithMaterialSlot(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, int32 MaterialSlotIndex, EPDModificationBoostType BoostType, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult) const;

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	bool TryModifyInventorySlot(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	bool TryModifyInventorySlotWithBoost(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, EPDModificationBoostType BoostType, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult);

	UFUNCTION(BlueprintCallable, Category = "PD|Equipment Modification")
	bool TryModifyInventorySlotWithMaterialSlot(UPDInventoryComponent* InventoryComponent, int32 InventorySlotIndex, int32 MaterialSlotIndex, EPDModificationBoostType BoostType, FPDModificationPreview& OutPreview, EPDModificationResult& OutResult);

protected:
	float GetCurveValue(FName RowName, int32 TargetLevel, float DefaultValue) const;
	float NormalizeSuccessRate(float RawRate) const;
	int32 GetGoldCostFromRecipe(int32 TargetLevel) const;
	TArray<FPDModificationMaterialRequirement> GetRequiredMaterials(int32 TargetLevel) const;
	bool GetBoostMaterial(EPDModificationBoostType BoostType, FName& OutItemID, int32& OutQuantity) const;
	float GetBoostRate(EPDModificationBoostType BoostType, int32 TargetLevel) const;
	void FillApplicableBonus(const FPDInventorySlot& TargetSlot, int32 TargetLevel, FPDModificationPreview& OutPreview) const;
	bool HasRequiredMaterials(const UPDInventoryComponent* InventoryComponent, const TArray<FPDModificationMaterialRequirement>& Materials) const;
	bool HasRequiredMaterialSlot(const UPDInventoryComponent* InventoryComponent, int32 MaterialSlotIndex, const TArray<FPDModificationMaterialRequirement>& Materials) const;
	bool ConsumeRequiredMaterials(UPDInventoryComponent* InventoryComponent, const TArray<FPDModificationMaterialRequirement>& Materials) const;
	bool ConsumeRequiredMaterialSlot(UPDInventoryComponent* InventoryComponent, int32 MaterialSlotIndex, const TArray<FPDModificationMaterialRequirement>& Materials) const;
};
