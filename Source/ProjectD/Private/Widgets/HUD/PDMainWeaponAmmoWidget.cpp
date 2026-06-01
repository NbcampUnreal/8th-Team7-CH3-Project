#include "Widgets/HUD/PDMainWeaponAmmoWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

#include "Characters/PDPlayerCharacter.h"
#include "Items/Equipment/PDEquipmentComponent.h"
#include "Weapons/Base/PDRangedWeaponBase.h"

void UPDMainWeaponAmmoWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	RefreshVisibility();
}

void UPDMainWeaponAmmoWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshAll();
}

void UPDMainWeaponAmmoWidget::NativeDestruct()
{
	UnbindWeapon();
	UnbindOwner();
	Super::NativeDestruct();
}

void UPDMainWeaponAmmoWidget::SetCaliberTextOverride(const FText& InText)
{
	CaliberOverride = InText;
	bCaliberOverrideSet = !InText.IsEmpty();
	RefreshMetadata();
}

void UPDMainWeaponAmmoWidget::SetSilhouetteOverride(UTexture2D* InTexture)
{
	SilhouetteOverride = InTexture;
	RefreshMetadata();
}

void UPDMainWeaponAmmoWidget::RefreshAll()
{
	EnsureOwnerBound();
	BindWeapon(ResolveCurrentRangedWeapon());
	RefreshAmmoText();
	RefreshMetadata();
	RefreshVisibility();
}

void UPDMainWeaponAmmoWidget::HandleWeaponFired(APDWeaponBase* /*Weapon*/)
{
	RefreshAmmoText();
}

void UPDMainWeaponAmmoWidget::HandleWeaponReloaded(APDWeaponBase* /*Weapon*/)
{
	RefreshAmmoText();
}

void UPDMainWeaponAmmoWidget::HandleWeaponAmmoChanged(APDWeaponBase* /*Weapon*/, int32 /*CurrentAmmo*/, int32 /*MaxAmmo*/, int32 /*AvailableAmmo*/, bool /*bIsReloading*/)
{
	RefreshAmmoText();
	RefreshVisibility();
}

void UPDMainWeaponAmmoWidget::HandleWeaponSwapped(APDWeaponBase* /*NewWeapon*/, EWeaponSlot /*WeaponSlot*/)
{
	BindWeapon(ResolveCurrentRangedWeapon());
	RefreshAmmoText();
	RefreshMetadata();
	RefreshVisibility();
}

void UPDMainWeaponAmmoWidget::EnsureOwnerBound()
{
	APDPlayerCharacter* Owner = FindOwnerPlayerCharacter();
	if (!Owner || BoundOwner.Get() == Owner)
	{
		return;
	}

	UnbindOwner();
	Owner->OnWeaponSwapped.AddDynamic(this, &UPDMainWeaponAmmoWidget::HandleWeaponSwapped);
	BoundOwner = Owner;
}

void UPDMainWeaponAmmoWidget::UnbindOwner()
{
	if (APDPlayerCharacter* Owner = BoundOwner.Get())
	{
		Owner->OnWeaponSwapped.RemoveDynamic(this, &UPDMainWeaponAmmoWidget::HandleWeaponSwapped);
	}
	BoundOwner.Reset();
}

void UPDMainWeaponAmmoWidget::BindWeapon(APDRangedWeaponBase* InWeapon)
{
	if (BoundWeapon.Get() == InWeapon)
	{
		return;
	}

	UnbindWeapon();

	if (!InWeapon)
	{
		return;
	}

	InWeapon->OnWeaponFired.AddDynamic(this, &UPDMainWeaponAmmoWidget::HandleWeaponFired);
	InWeapon->OnWeaponReloaded.AddDynamic(this, &UPDMainWeaponAmmoWidget::HandleWeaponReloaded);
	InWeapon->OnWeaponAmmoChanged.AddDynamic(this, &UPDMainWeaponAmmoWidget::HandleWeaponAmmoChanged);
	BoundWeapon = InWeapon;
}

void UPDMainWeaponAmmoWidget::UnbindWeapon()
{
	if (APDRangedWeaponBase* Weapon = BoundWeapon.Get())
	{
		Weapon->OnWeaponFired.RemoveDynamic(this, &UPDMainWeaponAmmoWidget::HandleWeaponFired);
		Weapon->OnWeaponReloaded.RemoveDynamic(this, &UPDMainWeaponAmmoWidget::HandleWeaponReloaded);
		Weapon->OnWeaponAmmoChanged.RemoveDynamic(this, &UPDMainWeaponAmmoWidget::HandleWeaponAmmoChanged);
	}
	BoundWeapon.Reset();
}

void UPDMainWeaponAmmoWidget::RefreshAmmoText()
{
	APDRangedWeaponBase* Weapon = BoundWeapon.Get();
	if (!Weapon)
	{
		if (Text_CurrentAmmo) Text_CurrentAmmo->SetText(FText::GetEmpty());
		if (Text_ReserveAmmo) Text_ReserveAmmo->SetText(FText::GetEmpty());
		if (Text_Combined)    Text_Combined->SetText(FText::GetEmpty());
		return;
	}

	const int32 Current = Weapon->GetCurrentAmmo();
	const int32 RawReserve = Weapon->GetAvailableAmmoCount();
	const bool bUnlimitedReserve = Weapon->HasInfiniteAmmo() || RawReserve == INT32_MAX;

	if (Text_CurrentAmmo)
	{
		Text_CurrentAmmo->SetText(FText::AsNumber(Current));
	}

	if (Text_ReserveAmmo)
	{
		const FString ReserveStr = bUnlimitedReserve
			? FString::Printf(TEXT("%s--"), *ReservePrefix)
			: FString::Printf(TEXT("%s%d"), *ReservePrefix, RawReserve);
		Text_ReserveAmmo->SetText(FText::FromString(ReserveStr));
	}

	if (Text_Combined)
	{
		const FString Combined = bUnlimitedReserve
			? FString::Printf(TEXT("%d / --"), Current)
			: FString::Printf(TEXT("%d / %d"), Current, RawReserve);
		Text_Combined->SetText(FText::FromString(Combined));
	}
}

void UPDMainWeaponAmmoWidget::RefreshMetadata()
{
	const FPDInventorySlot WeaponSlot = ResolveEquippedWeaponSlot();
	const bool bHasSlot = !WeaponSlot.IsEmpty();

	if (Text_Caliber)
	{
		if (bCaliberOverrideSet)
		{
			Text_Caliber->SetText(CaliberOverride);
		}
		else
		{
			// ?ë™ ?´ì„ ?°ì´???ŒìŠ¤ê°€ ?„ì§ ?†ìŒ ??ë¹??ìŠ¤?? ?„ìš” ??setterë¡???–´?°ê¸°.
			Text_Caliber->SetText(FText::GetEmpty());
		}
	}

	if (Text_ModLevel)
	{
		if (bHasSlot && WeaponSlot.ModificationLevel > 0)
		{
			Text_ModLevel->SetText(FText::AsNumber(WeaponSlot.ModificationLevel));
			Text_ModLevel->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Text_ModLevel->SetText(FText::GetEmpty());
			Text_ModLevel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// ?¤ë£¨???´ì„ ?°ì„ ?œìœ„: ?¸ë? Override > ë¬´ê¸° BP ??UISilhouette > Collapsed.
	if (Image_Silhouette)
	{
		UTexture2D* Resolved = SilhouetteOverride;
		if (!Resolved)
		{
			if (APDRangedWeaponBase* Weapon = BoundWeapon.Get())
			{
				Resolved = Weapon->GetUISilhouette();
			}
		}

		if (Resolved)
		{
			Image_Silhouette->SetBrushFromTexture(Resolved);
			Image_Silhouette->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Image_Silhouette->SetBrushFromTexture(nullptr);
			Image_Silhouette->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UPDMainWeaponAmmoWidget::RefreshVisibility()
{
	const bool bShow = BoundWeapon.IsValid();
	SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

APDPlayerCharacter* UPDMainWeaponAmmoWidget::FindOwnerPlayerCharacter() const
{
	return Cast<APDPlayerCharacter>(GetOwningPlayerPawn());
}

APDRangedWeaponBase* UPDMainWeaponAmmoWidget::ResolveCurrentRangedWeapon() const
{
	APDPlayerCharacter* Owner = FindOwnerPlayerCharacter();
	if (!Owner)
	{
		return nullptr;
	}
	return Cast<APDRangedWeaponBase>(Owner->GetCurrentWeapon());
}

UPDEquipmentComponent* UPDMainWeaponAmmoWidget::FindEquipmentComponent() const
{
	APDPlayerCharacter* Owner = FindOwnerPlayerCharacter();
	return Owner ? Owner->GetEquipmentComponent() : nullptr;
}

FPDInventorySlot UPDMainWeaponAmmoWidget::ResolveEquippedWeaponSlot() const
{
	if (UPDEquipmentComponent* Eq = FindEquipmentComponent())
	{
		return Eq->GetEquippedSlot(EPDEquipmentSlotType::Weapon);
	}
	return FPDInventorySlot();
}
