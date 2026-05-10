#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Type/Types.h"
#include "PDAnimInstance.generated.h"

class APDPlayerCharacter;
class APDWeaponBase;
class UAbilitySystemComponent;

USTRUCT()
struct FPDAnimInstanceCache
{
	GENERATED_BODY()

	float MovementSpeed=0.f;
	float Direction=0.f;
	bool bIsAiming=false;
	bool bIsJumping=false;
	float AimYaw=0.f;
	float AimPitch=0.f;

	EWeaponType WeaponType=EWeaponType::None;
	
	FVector LeftHandIKTarget=FVector::ZeroVector;
	float  LeftHandIKAlpha=0.f;
	
};

UCLASS()
class PROJECTD_API UPDAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	float MovementSpeed=0.f;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	float Direction=0.f;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	bool bIsAiming=false;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	bool bIsJumping=false;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	float AimYaw=0.f;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	float AimPitch=0.f;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	EWeaponType WeaponType=EWeaponType::None;

	UPROPERTY(BlueprintReadOnly, Category="Animation|IK")
	FVector LeftHandIKTarget=FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Animation|IK")
	float LeftHandIKAlpha=0.f;

private:
	UPROPERTY()
	TObjectPtr<APDPlayerCharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;

	FPDAnimInstanceCache Cache;
};