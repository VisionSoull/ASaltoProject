#include "PatrolAIController.h"
#include "NavigationSystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

APatrolAIController::APatrolAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	CurrentState = EAIState::Patrolling;
	TimeSinceLastChaseUpdate = 0.0f;

	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

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

void APatrolAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AIPerceptionComp)
	{
		AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &APatrolAIController::OnTargetPerceptionUpdated);
	}

	PatrolOrigin = InPawn->GetActorLocation();

	FTimerHandle StartPatrolTimer;
	GetWorldTimerManager().SetTimer(
		StartPatrolTimer,
		this,
		&APatrolAIController::StartPatrol,
		0.5f,
		false
	);
}

void APatrolAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState == EAIState::Chasing && TargetPlayer)
	{
		TimeSinceLastChaseUpdate += DeltaTime;

		if (TimeSinceLastChaseUpdate >= ChaseUpdateInterval)
		{
			ChasePlayer(TargetPlayer);
			TimeSinceLastChaseUpdate = 0.0f;
		}
	}
}

void APatrolAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor)
		return;

	APawn* SeenPawn = Cast<APawn>(Actor);
	if (SeenPawn && SeenPawn->IsPlayerControlled())
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			CurrentState = EAIState::Chasing;
			TargetPlayer = SeenPawn;
			GetWorldTimerManager().ClearTimer(PatrolTimerHandle);
			ChasePlayer(SeenPawn);
		}
		else
		{
			if (TargetPlayer == SeenPawn)
			{
				StartPatrol();
			}
		}
	}
}

void APatrolAIController::StartPatrol()
{
	CurrentState = EAIState::Patrolling;
	TargetPlayer = nullptr;
	MoveToRandomLocation();
}

void APatrolAIController::MoveToRandomLocation()
{
	if (CurrentState != EAIState::Patrolling)
		return;

	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSystem)
		return;

	FNavLocation RandomLocation;
	if (NavSystem->GetRandomPointInNavigableRadius(PatrolOrigin, PatrolRadius, RandomLocation))
	{
		MoveToLocation(RandomLocation.Location);

		GetWorldTimerManager().SetTimer(
			PatrolTimerHandle,
			this,
			&APatrolAIController::MoveToRandomLocation,
			PatrolWaitTime,
			false
		);
	}
}

void APatrolAIController::ChasePlayer(APawn* Player)
{
	if (!Player)
	{
		StartPatrol();
		return;
	}

	MoveToActor(Player);
}