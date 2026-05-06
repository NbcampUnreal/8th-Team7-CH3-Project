#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Type/Types.h"
#include "PDBodyPartConfig.generated.h"



UCLASS(BlueprintType)
class PROJECTD_API UPDBodyPartConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|BodyPart")
	TArray<FBodyPartMapping> Mappings;

	UFUNCTION(BlueprintCallable, Category = "PD|BodyPart")
	EBodyPart GetBodyPartFromName(FName InName) const;
};

