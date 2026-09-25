#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "StealthTestCharacter.generated.h"

/**
 * Minimal floating body for an enemy AI: no gravity, no collision, just a visible shape you
 * place in the level. Auto-possessed by AStealthTestAIController at BeginPlay (AIControllerClass
 * + AutoPossessAI below) - just place this, no separate controller actor needed.
 *
 * All detection logic and tuning (SightRadius/PeripheralVisionAngleDegrees/etc) stays on the
 * controller, per usual AI convention - this pawn only carries the editor-only arrow/sight-cone
 * debug visuals, since AIPerceptionComponent senses from the POSSESSED PAWN's transform, not the
 * controller's own (AAIController never moves to match its pawn's location). The controller pushes
 * its current sight values into RefreshSightVisual() once it possesses this pawn, so the cone
 * always reflects whatever the controller is actually using.
 * Not meant to survive past testing, replace with the real enemy pawn once that exists.
 */
UCLASS()
class ASALTOPROJECT_API AStealthTestCharacter : public APawn
{
	GENERATED_BODY()

public:
	AStealthTestCharacter();

	// Called by AStealthTestAIController::OnPossess so the debug cone matches its actual sight config.
	void RefreshSightVisual(float NewSightRadius, float NewPeripheralVisionAngleDegrees);

private:
	// Plain scene root, no collision of its own.
	UPROPERTY(VisibleAnywhere, Category = "Stealth|Test")
	class USceneComponent* SceneRoot;

	// Purely visual, collision disabled so it can never block or be blocked by anything.
	UPROPERTY(VisibleAnywhere, Category = "Stealth|Test")
	class UStaticMeshComponent* VisualMesh;

	// Floating movement: no gravity, no capsule. Not driven by anything yet, here so a future
	// patrol/AI Move To behavior has something to move with.
	UPROPERTY(VisibleAnywhere, Category = "Stealth|Test")
	class UFloatingPawnMovement* MovementComponent;

	// Editor-only visuals so you can see facing + sight cone right where sensing actually
	// happens, with no Play required. Never show up in a packaged build.
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class UArrowComponent* ArrowComponent;

	UPROPERTY()
	class UStealthSightConeComponent* SightConeComponent;
#endif
};
