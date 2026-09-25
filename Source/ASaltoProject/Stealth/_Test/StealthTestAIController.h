#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "Perception/AIPerceptionTypes.h"
#include "StealthTestAIController.generated.h"

/**
 * Minimal AI controller for testing the stealth detection pipeline end to end: senses the
 * player via AIPerceptionComponent's Sight sense, then calls ReportDetection() on the
 * player's UStealthComponent. All detection logic and tuning lives here, as usual for AI
 * (Controller = brain, Pawn = body). Meant to possess an AStealthTestCharacter (via that
 * pawn's AIControllerClass/AutoPossessAI) rather than be placed in the level itself; on
 * possession it pushes its sight values into that pawn's debug cone purely so you can see
 * where it's looking, since AIPerceptionComponent senses from the possessed pawn's transform.
 * Not meant to survive past testing, replace with the real enemy AI controller(s) once those exist.
 */
UCLASS()
class ASALTOPROJECT_API AStealthTestAIController : public AAIController
{
	GENERATED_BODY()

public:
	AStealthTestAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UPROPERTY(EditAnywhere, Category = "Stealth|Test")
	float SightRadius = 1500.0f;

	UPROPERTY(EditAnywhere, Category = "Stealth|Test")
	float LoseSightRadius = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Stealth|Test")
	float PeripheralVisionAngleDegrees = 90.0f;

	// Reason tag passed along to ReportDetection(). Set this in the editor once your team
	// has actual detection tags defined, leaving it unset still works fine for testing.
	UPROPERTY(EditAnywhere, Category = "Stealth|Test")
	FGameplayTag SightDetectionReason;

	UPROPERTY()
	class UAIPerceptionComponent* AIPerceptionComp;

	UPROPERTY()
	class UAISenseConfig_Sight* SightConfig;
};
