#include "Enemy/Characters/PDBaseNoiseEmitter.h"

#include "Engine/World.h"
#include "Perception/AISense_Hearing.h"

APDBaseNoiseEmitter::APDBaseNoiseEmitter()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APDBaseNoiseEmitter::EmitNoise()
{
	const float NoiseRange = MaxNoiseRange + FMath::RandRange(-NoiseRangeRandomDeviation, NoiseRangeRandomDeviation);
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.f, this, NoiseRange, NoiseTag);
}
