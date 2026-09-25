#include "StealthTestCharacter.h"
#include "StealthTestAIController.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "UObject/ConstructorHelpers.h"
#if WITH_EDITORONLY_DATA
#include "Components/ArrowComponent.h"
#include "StealthSightConeComponent.h"
#endif

AStealthTestCharacter::AStealthTestCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// Auto-spawns and possesses a StealthTestAIController at BeginPlay - just place this pawn,
	// no need to separately place a controller actor in the level.
	AIControllerClass = AStealthTestAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	VisualMesh->SetGenerateOverlapEvents(false);
	VisualMesh->SetCanEverAffectNavigation(false);

	// Engine's basic sphere as a placeholder so the pawn is visible in the viewport/PIE.
	// Swap for a real mesh later, this is only here so you have something to see and place.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		VisualMesh->SetStaticMesh(SphereMeshAsset.Object);
		VisualMesh->SetRelativeScale3D(FVector(0.5f)); // ~50cm sphere, roughly head-height marker
	}

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->MaxSpeed = 600.0f;
	MovementComponent->Acceleration = 4000.0f;
	MovementComponent->Deceleration = 4000.0f;
	
#if WITH_EDITORONLY_DATA
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (ArrowComponent)
	{
		ArrowComponent->ArrowColor = FColor(255, 60, 60);
		ArrowComponent->ArrowSize = 1.5f;
		ArrowComponent->bTreatAsASprite = true;
		ArrowComponent->bIsScreenSizeScaled = true;
		ArrowComponent->SetupAttachment(SceneRoot);
	}

	SightConeComponent = CreateEditorOnlyDefaultSubobject<UStealthSightConeComponent>(TEXT("SightConeVisualizer"));
	if (SightConeComponent)
	{
		SightConeComponent->SetupAttachment(SceneRoot);
		// Shows the component's own defaults (1500/90) until a controller possesses this pawn and
		// calls RefreshSightVisual() with its real values.
	}
#endif
}

void AStealthTestCharacter::RefreshSightVisual(float NewSightRadius, float NewPeripheralVisionAngleDegrees)
{
#if WITH_EDITORONLY_DATA
	if (SightConeComponent)
	{
		SightConeComponent->RefreshConeParameters(NewSightRadius, NewPeripheralVisionAngleDegrees);
	}
#endif
}
