#include "SimpleScanningAIController.h"
#include "Perception/PawnSensingComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

ASimpleScanningAIController::ASimpleScanningAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	PawnSensingComp = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensingComponent"));
	PawnSensingComp->bOnlySensePlayers = true;

	CurrentState = EScanningAIState::Idle;
	TargetPlayer = nullptr;
	CurrentScanAngle = 0.0f;
	bScanningRight = true;
	PauseTimer = 0.0f;
	bInteractionTriggered = false;
}

void ASimpleScanningAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	SpawnLocation = InPawn->GetActorLocation();
	InitialRotation = InPawn->GetActorRotation();

	if (PawnSensingComp)
	{
		PawnSensingComp->SightRadius = SightRadius;
		PawnSensingComp->HearingThreshold = HearingThreshold;
		PawnSensingComp->SetPeripheralVisionAngle(PeripheralVisionAngle);
		PawnSensingComp->bSeePawns = true;
		PawnSensingComp->bHearNoises = false;
		PawnSensingComp->OnSeePawn.AddDynamic(this, &ASimpleScanningAIController::OnSeePawn);

		UE_LOG(LogTemp, Log, TEXT("SimpleScanningAI: PawnSensing configured - Sight: %.0f, Hearing: %.0f, FOV: %.0f"),
			SightRadius, HearingThreshold, PeripheralVisionAngle);
	}

	if (bEnableAvoidance)
	{
		ACharacter* ControlledCharacter = Cast<ACharacter>(InPawn);
		if (ControlledCharacter && ControlledCharacter->GetCharacterMovement())
		{
			UCharacterMovementComponent* MovementComp = ControlledCharacter->GetCharacterMovement();
			MovementComp->bUseRVOAvoidance = true;
			MovementComp->AvoidanceWeight = AvoidanceWeight;
			MovementComp->AvoidanceConsiderationRadius = 500.0f;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("SimpleScanningAI: Initialized at location %s"), *SpawnLocation.ToString());
}

void ASimpleScanningAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState == EScanningAIState::Idle)
	{
		UpdateScanning(DeltaTime);
	}
	else if (CurrentState == EScanningAIState::Chasing && TargetPlayer)
	{
		CheckInteractionDistance();

		float DistanceToTarget = FVector::Dist(GetPawn()->GetActorLocation(), TargetPlayer->GetActorLocation());
		if (DistanceToTarget > LoseTargetDistance)
		{
			UE_LOG(LogTemp, Log, TEXT("SimpleScanningAI: Lost target, returning to spawn"));
			StartReturning();
		}
	}
	else if (CurrentState == EScanningAIState::Returning)
	{
		float DistanceToSpawn = FVector::Dist(GetPawn()->GetActorLocation(), SpawnLocation);
		if (DistanceToSpawn <= ReturnAcceptanceRadius)
		{
			CurrentState = EScanningAIState::Idle;
			StopMovement();

			// Reset scanning state
			CurrentScanAngle = 0.0f;
			bScanningRight = true;
			PauseTimer = 0.0f;
			GetPawn()->SetActorRotation(InitialRotation);

			UE_LOG(LogTemp, Log, TEXT("SimpleScanningAI: Returned to spawn, resuming scanning"));
		}
	}
}

void ASimpleScanningAIController::OnSeePawn(APawn* SeenPawn)
{
	if (!SeenPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("SimpleScanningAI: OnSeePawn called with null pawn"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("SimpleScanningAI: Saw pawn %s (IsPlayerControlled: %s, CurrentState: %d)"),
		*SeenPawn->GetName(),
		SeenPawn->IsPlayerControlled() ? TEXT("true") : TEXT("false"),
		(int32)CurrentState);

	if (!SeenPawn->IsPlayerControlled())
		return;

	if (CurrentState == EScanningAIState::Idle)
	{
		UE_LOG(LogTemp, Log, TEXT("SimpleScanningAI: Player detected, starting chase"));
		StartChasing(SeenPawn);
	}
}

void ASimpleScanningAIController::StartChasing(APawn* Target)
{
	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("SimpleScanningAI: StartChasing called with null target"));
		return;
	}

	CurrentState = EScanningAIState::Chasing;
	TargetPlayer = Target;
	bInteractionTriggered = false;

	UE_LOG(LogTemp, Log, TEXT("SimpleScanningAI: Starting chase of %s at distance %.2f"),
		*Target->GetName(),
		FVector::Dist(GetPawn()->GetActorLocation(), Target->GetActorLocation()));

	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalActor(Target);
	MoveRequest.SetAcceptanceRadius(ChaseAcceptanceRadius);
	MoveRequest.SetUsePathfinding(true);

	FNavPathSharedPtr NavPath;
	FPathFollowingRequestResult Result = MoveTo(MoveRequest, &NavPath);

	UE_LOG(LogTemp, Log, TEXT("SimpleScanningAI: MoveTo result - Code: %d"), (int32)Result.Code);
}

void ASimpleScanningAIController::CheckInteractionDistance()
{
	if (!TargetPlayer || bInteractionTriggered)
		return;

	float Distance = FVector::Dist(GetPawn()->GetActorLocation(), TargetPlayer->GetActorLocation());

	if (Distance <= InteractionDistance)
	{
		CurrentState = EScanningAIState::Interacting;
		StopMovement();
		bInteractionTriggered = true;

		UE_LOG(LogTemp, Log, TEXT("SimpleScanningAI: Reached interaction distance, triggering event"));
		OnPlayerReached(TargetPlayer);
	}
}

void ASimpleScanningAIController::CompleteInteraction()
{
	UE_LOG(LogTemp, Log, TEXT("SimpleScanningAI: Interaction complete, returning to spawn"));
	StartReturning();
}

void ASimpleScanningAIController::StartReturning()
{
	CurrentState = EScanningAIState::Returning;
	TargetPlayer = nullptr;
	bInteractionTriggered = false;

	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(SpawnLocation);
	MoveRequest.SetAcceptanceRadius(ReturnAcceptanceRadius);
	MoveRequest.SetUsePathfinding(true);

	FNavPathSharedPtr NavPath;
	MoveTo(MoveRequest, &NavPath);
}

void ASimpleScanningAIController::UpdateScanning(float DeltaTime)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
		return;

	if (PauseTimer > 0.0f)
	{
		PauseTimer -= DeltaTime;
		return;
	}

	float DeltaRotation = ScanRotationSpeed * DeltaTime;

	if (bScanningRight)
	{
		CurrentScanAngle += DeltaRotation;
		if (CurrentScanAngle >= ScanAngleRange)
		{
			CurrentScanAngle = ScanAngleRange;
			bScanningRight = false;
			PauseTimer = PauseDurationAtEdge;
		}
	}
	else
	{
		CurrentScanAngle -= DeltaRotation;
		if (CurrentScanAngle <= -ScanAngleRange)
		{
			CurrentScanAngle = -ScanAngleRange;
			bScanningRight = true;
			PauseTimer = PauseDurationAtEdge;
		}
	}

	FRotator NewRotation = InitialRotation;
	NewRotation.Yaw += CurrentScanAngle;
	ControlledPawn->SetActorRotation(NewRotation);
}