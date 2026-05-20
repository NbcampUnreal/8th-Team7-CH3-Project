#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Type/Types.h"
#include "PDMainWeaponAmmoWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;
class APDPlayerCharacter;
class APDWeaponBase;
class APDRangedWeaponBase;
class UPDEquipmentComponent;

/**
 * 현재 들고 있는 무기(Main Weapon)의 탄약 readout.
 * APDPlayerCharacter::OnWeaponSwapped + RangedWeapon::OnWeaponFired/Reloaded 푸시 모델.
 * 비-원거리 무기(근접/None)이거나 무기 없음 → Collapsed.
 * 모든 BindWidget* 필드는 Optional — 디자이너가 원하는 조합만 노출 가능.
 * 디자이너/외부 코드가 텍스트/실루엣을 직접 덮어쓸 수 있도록 setter 노출(확장성).
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTD_API UPDMainWeaponAmmoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 외부에서 구경 텍스트를 강제 지정. 비우면 자동 해석값 사용. */
	UFUNCTION(BlueprintCallable, Category="PD|MainWeaponAmmo")
	void SetCaliberTextOverride(const FText& InText);

	/** 외부에서 실루엣 텍스처를 강제 지정. nullptr이면 자동 해석값 사용. */
	UFUNCTION(BlueprintCallable, Category="PD|MainWeaponAmmo")
	void SetSilhouetteOverride(UTexture2D* InTexture);

	/** 현재 바인딩된 메인 무기. 없으면 nullptr. */
	UFUNCTION(BlueprintPure, Category="PD|MainWeaponAmmo")
	APDRangedWeaponBase* GetBoundWeapon() const { return BoundWeapon.Get(); }

	/** 강제 갱신. 디자이너/외부 변동 후 호출. */
	UFUNCTION(BlueprintCallable, Category="PD|MainWeaponAmmo")
	void RefreshAll();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ── BindWidgetOptional 텍스트 슬롯 ──
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CurrentAmmo;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ReserveAmmo;

	/** "current / reserve" 한 줄 표시용 (위 두 개 대신 단일 텍스트 선호 시). */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Combined;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Caliber;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ModLevel;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_Silhouette;

	/** 예비탄 표시 prefix (예: "/ ") — Text_ReserveAmmo만 사용할 때 슬래시 포함 표기. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PD|MainWeaponAmmo")
	FString ReservePrefix = TEXT("/ ");

private:
	UFUNCTION()
	void HandleWeaponFired(APDWeaponBase* Weapon);

	UFUNCTION()
	void HandleWeaponReloaded(APDWeaponBase* Weapon);

	UFUNCTION()
	void HandleWeaponSwapped(APDWeaponBase* NewWeapon, EWeaponSlot WeaponSlot);

	void EnsureOwnerBound();
	void UnbindOwner();
	void BindWeapon(APDRangedWeaponBase* InWeapon);
	void UnbindWeapon();

	void RefreshAmmoText();
	void RefreshMetadata();
	void RefreshVisibility();

	APDPlayerCharacter* FindOwnerPlayerCharacter() const;
	APDRangedWeaponBase* ResolveCurrentRangedWeapon() const;
	UPDEquipmentComponent* FindEquipmentComponent() const;
	FPDInventorySlot ResolveEquippedWeaponSlot() const;

	TWeakObjectPtr<APDPlayerCharacter> BoundOwner;
	TWeakObjectPtr<APDRangedWeaponBase> BoundWeapon;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> SilhouetteOverride = nullptr;

	FText CaliberOverride;
	bool bCaliberOverrideSet = false;
};