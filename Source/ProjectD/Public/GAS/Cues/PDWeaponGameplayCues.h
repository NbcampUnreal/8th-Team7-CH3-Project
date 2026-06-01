#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "PDWeaponGameplayCues.generated.h"

class USoundBase;
class UNiagaraSystem;
class UParticleSystem;

UCLASS(Blueprintable)
class PROJECTD_API UGCN_Weapon_Fire : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	UGCN_Weapon_Fire();
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};

UCLASS(Blueprintable)
class PROJECTD_API UGCN_Weapon_Impact : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	UGCN_Weapon_Impact();
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};

UCLASS(Blueprintable)
class PROJECTD_API UGCN_Weapon_Equip : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	UGCN_Weapon_Equip();
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};

UCLASS(Blueprintable)
class PROJECTD_API UGCN_Weapon_Swing : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	UGCN_Weapon_Swing();
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};

UCLASS(Blueprintable)
class PROJECTD_API UGCN_Weapon_MeleeHit : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	UGCN_Weapon_MeleeHit();
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};


UCLASS(Blueprintable)
class PROJECTD_API UGCN_Character_HitReact : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	UGCN_Character_HitReact();
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};


UCLASS(Blueprintable)
class PROJECTD_API UGCN_Character_Bleeding : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	UGCN_Character_Bleeding();
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};

UCLASS(Blueprintable)
class PROJECTD_API UGCN_Character_Roll : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	UGCN_Character_Roll();
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};

UCLASS(Blueprintable)
class PROJECTD_API UGCN_Character_Footstep : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	UGCN_Character_Footstep();
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};

UCLASS(Blueprintable)
class PROJECTD_API UGCN_Weapon_CartridgeHit : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	UGCN_Weapon_CartridgeHit();
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};

UCLASS(Blueprintable)
class PROJECTD_API UGCN_Weapon_Reload : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	UGCN_Weapon_Reload();
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<USoundBase> ReloadSound;
};

UCLASS(Blueprintable)
class PROJECTD_API UGCN_Weapon_ReloadEmpty : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	UGCN_Weapon_ReloadEmpty();
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|FX")
	TObjectPtr<USoundBase> ReloadEmptySound;
};

UCLASS(Blueprintable)
class PROJECTD_API UGCN_Item_Pickup : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	UGCN_Item_Pickup();
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|FX")
	TObjectPtr<USoundBase> PickupSound;
};

UCLASS(Blueprintable)
class PROJECTD_API UGCN_Ability_Bombing : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	UGCN_Ability_Bombing();
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Bombing|FX")
	TObjectPtr<UNiagaraSystem> BombingNiagara;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Bombing|FX")
	TObjectPtr<UParticleSystem> BombingParticle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Bombing|FX")
	TObjectPtr<USoundBase> BombingSound;
};
