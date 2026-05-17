#include "Enemy/Components/PDPerceptionComponent.h"

#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

UPDPerceptionComponent::UPDPerceptionComponent()
{
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("PDSightConfig"));
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("PDHearingConfig"));

	// 친화도 필터는 ConfigureSense 이전에 확정해야 sense 등록 시 올바르게 반영됨.
	// (엔진 디폴트는 셋 다 true 라 사후 변경이 안 먹는 케이스가 있음 — 같은 팀끼리도 spot 됨.)
	if (SightConfig)
	{
		SightConfig->DetectionByAffiliation.bDetectEnemies    = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals   = false;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
		ConfigureSense(*SightConfig);
		SetDominantSense(SightConfig->GetSenseImplementation());
	}
	if (HearingConfig)
	{
		HearingConfig->DetectionByAffiliation.bDetectEnemies    = true;
		HearingConfig->DetectionByAffiliation.bDetectNeutrals   = false;
		HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;
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
	}

	if (HearingConfig)
	{
		HearingConfig->HearingRange = HearingRange;
		HearingConfig->SetMaxAge(HearingMaxAge);
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
