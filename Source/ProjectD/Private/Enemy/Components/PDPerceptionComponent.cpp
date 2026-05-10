#include "Enemy/Components/PDPerceptionComponent.h"

#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

UPDPerceptionComponent::UPDPerceptionComponent()
{
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("PDSightConfig"));
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("PDHearingConfig"));

	// ConfigureSense 는 생성자에서 호출되어야 함 — OnRegister 가 SensesConfig 배열을 listener 에 매핑.
	if (SightConfig)
	{
		ConfigureSense(*SightConfig);
		SetDominantSense(SightConfig->GetSenseImplementation());
	}
	if (HearingConfig)
	{
		ConfigureSense(*HearingConfig);
	}
}

void UPDPerceptionComponent::BeginPlay()
{
	Super::BeginPlay();

	// BP defaults 의 EditDefaultsOnly 값을 SenseConfig 에 적용 (생성자에서는 BP serialize 전).
	if (SightConfig)
	{
		SightConfig->SightRadius = SightRadius;
		SightConfig->LoseSightRadius = FMath::Max(LoseSightRadius, SightRadius);
		SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
		SightConfig->SetMaxAge(SightMaxAge);
		SightConfig->DetectionByAffiliation.bDetectEnemies    = bDetectEnemies;
		SightConfig->DetectionByAffiliation.bDetectNeutrals   = bDetectNeutrals;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = bDetectFriendlies;
	}

	if (HearingConfig)
	{
		HearingConfig->HearingRange = HearingRange;
		HearingConfig->SetMaxAge(HearingMaxAge);
		HearingConfig->DetectionByAffiliation.bDetectEnemies    = bDetectEnemies;
		HearingConfig->DetectionByAffiliation.bDetectNeutrals   = bDetectNeutrals;
		HearingConfig->DetectionByAffiliation.bDetectFriendlies = bDetectFriendlies;
	}

	RequestStimuliListenerUpdate();

	OnTargetPerceptionUpdated.AddDynamic(this, &UPDPerceptionComponent::HandlePerceptionUpdated);
}

void UPDPerceptionComponent::HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;

	OnTargetSensed.Broadcast(Actor, Stimulus.WasSuccessfullySensed());

	const FAISenseID SenseID = Stimulus.Type;
	const FAISenseID SightID = UAISense::GetSenseID(UAISense_Sight::StaticClass());
	const FAISenseID HearingID = UAISense::GetSenseID(UAISense_Hearing::StaticClass());

	if (SenseID == SightID)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			OnTargetSpotted.Broadcast(Actor);
		}
		else
		{
			OnTargetLost.Broadcast(Actor, Stimulus.StimulusLocation);
		}
	}
	else if (SenseID == HearingID)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			OnNoiseHeard.Broadcast(Actor, Stimulus.StimulusLocation);
		}
	}
}
