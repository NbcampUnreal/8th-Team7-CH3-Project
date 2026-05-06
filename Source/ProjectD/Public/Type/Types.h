#pragma once

#include "CoreMinimal.h"
#include "Types.generated.h"

USTRUCT(BlueprintType)
struct FPDDamageInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<AActor> Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UDamageType> DamageTypeClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHitResult HitResult;
};

UENUM(BlueprintType)
enum class EPDEnemyState : uint8
{
	Idle,
	Chase,
	Attack,
	Dead
};

UENUM(BlueprintType)
enum class EBodyPart : uint8
{
	None,
	Head,
	Torso,
	Arm_L,
	Arm_R,
	Leg_L,
	Leg_R
};

USTRUCT(BlueprintType)
struct FBodyPartMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName HitBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EBodyPart BodyPart=EBodyPart::None;
};

UENUM(BlueprintType)
enum class ERaidState : uint8
{
	Idle,
	InProgress,
	Extracting,
	Ended
};

USTRUCT(BlueprintType)
struct FPDPlayerData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Gold=0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Experience=0;
	
	//Inventory Data
	
	//Current Equppied Weapon
	
	
	
};

