#include "StealthTestAIController.h"
#include "../StealthComponent.h"
#include "StealthTestCharacter.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AStealthTestAIController::AStealthTestAIController()
{
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*AIPerceptionComp);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
	SightConfig->SetMaxAge(5.0f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	AIPerceptionComp->ConfigureSense(*SightConfig);
	AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AStealthTestAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AIPerceptionComp)
	{
		AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AStealthTestAIController::OnTargetPerceptionUpdated);
	}

	// Purely cosmetic: push our sight values into the pawn's debug cone so you can see where
	// this controller is looking. Detection itself doesn't depend on this in any way.
	if (AStealthTestCharacter* TestCharacter = Cast<AStealthTestCharacter>(InPawn))
	{
		TestCharacter->RefreshSightVisual(SightRadius, PeripheralVisionAngleDegrees);
	}
}

void AStealthTestAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !Stimulus.WasSuccessfullySensed())
	{
		return;
	}

	APawn* SeenPawn = Cast<APawn>(Actor);
	if (!SeenPawn || !SeenPawn->IsPlayerControlled())
	{
		return;
	}

	if (UStealthComponent* StealthComp = SeenPawn->FindComponentByClass<UStealthComponent>())
	{
		StealthComp->ReportDetection(GetPawn(), SightDetectionReason, Stimulus.StimulusLocation);
	}
}
