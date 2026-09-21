#include "PoliceAIController.h"
#include "PolicePatrolManager.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

APoliceAIController::APoliceAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	CurrentState = EPoliceState::Patrolling;
	AssignedRouteIndex = 0;
	bPatrolClockwise = true;
	CurrentWaypointIndex = 0;
	CompletedPatrolCount = 0;
	TimeSinceLastUpdate = 0.0f;
	InvestigationTimer = 0.0f;
	MeetingWaitTimer = 0.0f;
	bWaitingForPartner = false;
	bInitialized = false;

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

void APoliceAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AIPerceptionComp)
	{
		AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &APoliceAIController::OnTargetPerceptionUpdated);
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

			UE_LOG(LogTemp, Log, TEXT("PoliceAIController: RVO Avoidance enabled for %s"), *InPawn->GetName());
		}
	}
}

void APoliceAIController::InitializeFromManager(APolicePatrolManager* Manager, int32 RouteIndex, bool bClockwise)
{
	PatrolManager = Manager;
	AssignedRouteIndex = RouteIndex;
	bPatrolClockwise = bClockwise;
	bInitialized = true;

	UE_LOG(LogTemp, Log, TEXT("PoliceAIController: Initialized for route %d (%s)"),
		RouteIndex, bClockwise ? TEXT("Clockwise") : TEXT("Counter-clockwise"));

	GetWorldTimerManager().SetTimer(
		WaypointWaitTimer,
		[this]() { StartPatrolRoute(); },
		0.5f,
		false
	);
}

void APoliceAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bInitialized)
		return;

	if (CurrentState == EPoliceState::Investigating)
	{
		UpdateInvestigation(DeltaTime);
	}
	else if (CurrentState == EPoliceState::Patrolling)
	{
		APawn* ControlledPawn = GetPawn();
		if (ControlledPawn && CurrentWaypoints.IsValidIndex(CurrentWaypointIndex))
		{
			float Distance = FVector::Dist(ControlledPawn->GetActorLocation(),
				CurrentWaypoints[CurrentWaypointIndex]->GetActorLocation());

			if (Distance <= WaypointReachedDistance)
			{
				OnWaypointReached();
			}
		}
	}
}

void APoliceAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !bInitialized)
		return;

	APawn* SeenPawn = Cast<APawn>(Actor);
	if (SeenPawn && SeenPawn->IsPlayerControlled())
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			if (CurrentState != EPoliceState::Investigating)
			{
				StartInvestigation(SeenPawn);
			}
		}
		else
		{
			if (CurrentState == EPoliceState::Investigating && InvestigationTarget == SeenPawn)
			{
				LastKnownTargetLocation = Stimulus.StimulusLocation;
			}
		}
	}
}

void APoliceAIController::StartPatrolRoute()
{
	if (!PatrolManager)
		return;

	CurrentWaypoints = PatrolManager->GetRouteWaypoints(AssignedRouteIndex);

	if (CurrentWaypoints.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("PoliceAIController: No waypoints found for route %d!"), AssignedRouteIndex);
		return;
	}

	CurrentWaypointIndex = bPatrolClockwise ? 0 : CurrentWaypoints.Num() - 1;
	CurrentState = EPoliceState::Patrolling;

	UE_LOG(LogTemp, Log, TEXT("PoliceAIController: Starting patrol on route %d (%s)"),
		AssignedRouteIndex, bPatrolClockwise ? TEXT("Clockwise") : TEXT("Counter-clockwise"));

	MoveToNextWaypoint();
}

void APoliceAIController::MoveToNextWaypoint()
{
	if (!CurrentWaypoints.IsValidIndex(CurrentWaypointIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("PoliceAIController: Invalid waypoint index!"));
		return;
	}

	APatrolWaypoint* TargetWaypoint = CurrentWaypoints[CurrentWaypointIndex];
	if (TargetWaypoint)
	{
		FAIMoveRequest MoveRequest;
		MoveRequest.SetGoalLocation(TargetWaypoint->GetActorLocation());
		MoveRequest.SetAcceptanceRadius(50.0f);
		MoveRequest.SetUsePathfinding(true);

		FNavPathSharedPtr NavPath;
		MoveTo(MoveRequest, &NavPath);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PoliceAIController: Waypoint is null!"));
	}
}

int32 APoliceAIController::GetNextWaypointIndex() const
{
	if (bPatrolClockwise)
	{
		return (CurrentWaypointIndex + 1) % CurrentWaypoints.Num();
	}
	else
	{
		return (CurrentWaypointIndex - 1 + CurrentWaypoints.Num()) % CurrentWaypoints.Num();
	}
}

void APoliceAIController::OnWaypointReached()
{
	if (!CurrentWaypoints.IsValidIndex(CurrentWaypointIndex))
		return;

	APatrolWaypoint* ReachedWaypoint = CurrentWaypoints[CurrentWaypointIndex];

	if (ReachedWaypoint->bIsMeetingPoint && !ReachedWaypoint->bCoordinateWithOtherRoutes)
	{
		CompletedPatrolCount++;
		UE_LOG(LogTemp, Log, TEXT("PoliceAIController: Reached route meeting point, completed patrol %d"), CompletedPatrolCount);

		if (PatrolManager && PatrolManager->ShouldMeetAfterPatrol(CompletedPatrolCount))
		{
			CurrentState = EPoliceState::WaitingAtMeeting;

			GetWorldTimerManager().SetTimer(
				WaypointWaitTimer,
				[this]()
				{
					TArray<int32> SingleRoute;
					SingleRoute.Add(AssignedRouteIndex);
					PerformSwap(SingleRoute);
				},
				ReachedWaypoint->WaitTime,
				false
			);
			return;
		}
	}

	CurrentState = EPoliceState::WaitingAtWaypoint;

	GetWorldTimerManager().SetTimer(
		WaypointWaitTimer,
		[this]()
		{
			int32 NextIndex = GetNextWaypointIndex();

			if ((bPatrolClockwise && NextIndex == 0) || (!bPatrolClockwise && NextIndex == CurrentWaypoints.Num() - 1))
			{
				CompletedPatrolCount++;
				UE_LOG(LogTemp, Log, TEXT("PoliceAIController: Completed patrol %d"), CompletedPatrolCount);

				if (PatrolManager && PatrolManager->ShouldMeetAfterPatrol(CompletedPatrolCount))
				{
					GoToMeetingPoint();
					return;
				}
			}

			CurrentWaypointIndex = NextIndex;
			CurrentState = EPoliceState::Patrolling;
			MoveToNextWaypoint();
		},
		ReachedWaypoint->WaitTime,
		false
	);
}

void APoliceAIController::PerformSwap(const TArray<int32>& ConnectedRoutes)
{
	if (!PatrolManager)
		return;

	if (PatrolManager->SwapMode == ESwapMode::SwapRoutes && ConnectedRoutes.Num() > 1)
	{
		int32 NewRouteIndex = PatrolManager->FindPartnerRouteIndex(AssignedRouteIndex, ConnectedRoutes);
		UE_LOG(LogTemp, Log, TEXT("PoliceAIController: Swapping routes from %d to %d"), AssignedRouteIndex, NewRouteIndex);
		AssignedRouteIndex = NewRouteIndex;
	}
	else
	{
		bPatrolClockwise = !bPatrolClockwise;
		UE_LOG(LogTemp, Log, TEXT("PoliceAIController: Swapping direction to %s"),
			bPatrolClockwise ? TEXT("Clockwise") : TEXT("Counter-clockwise"));
	}

	CompletedPatrolCount = 0;
	StartPatrolRoute();
}

void APoliceAIController::StartInvestigation(APawn* Target)
{
	GetWorldTimerManager().ClearTimer(WaypointWaitTimer);
	GetWorldTimerManager().ClearTimer(MeetingTimeoutTimer);

	CurrentState = EPoliceState::Investigating;
	InvestigationTarget = Target;
	InvestigationTimer = 0.0f;
	TimeSinceLastUpdate = 0.0f;

	UE_LOG(LogTemp, Log, TEXT("PoliceAIController: Starting investigation"));

	if (Target)
	{
		LastKnownTargetLocation = Target->GetActorLocation();

		FAIMoveRequest MoveRequest;
		MoveRequest.SetGoalActor(Target);
		MoveRequest.SetAcceptanceRadius(100.0f);
		MoveRequest.SetUsePathfinding(true);

		FNavPathSharedPtr NavPath;
		MoveTo(MoveRequest, &NavPath);
	}
}

void APoliceAIController::UpdateInvestigation(float DeltaTime)
{
	InvestigationTimer += DeltaTime;
	TimeSinceLastUpdate += DeltaTime;

	if (InvestigationTimer >= InvestigationTimeout)
	{
		EndInvestigation();
		return;
	}

	if (InvestigationTarget && TimeSinceLastUpdate >= InvestigationUpdateInterval)
	{
		FAIMoveRequest MoveRequest;
		MoveRequest.SetGoalActor(InvestigationTarget);
		MoveRequest.SetAcceptanceRadius(100.0f);
		MoveRequest.SetUsePathfinding(true);

		FNavPathSharedPtr NavPath;
		MoveTo(MoveRequest, &NavPath);

		TimeSinceLastUpdate = 0.0f;
	}
	else if (!InvestigationTarget && TimeSinceLastUpdate >= InvestigationUpdateInterval)
	{
		FAIMoveRequest MoveRequest;
		MoveRequest.SetGoalLocation(LastKnownTargetLocation);
		MoveRequest.SetAcceptanceRadius(100.0f);
		MoveRequest.SetUsePathfinding(true);

		FNavPathSharedPtr NavPath;
		MoveTo(MoveRequest, &NavPath);

		TimeSinceLastUpdate = 0.0f;
	}
}

void APoliceAIController::EndInvestigation()
{
	UE_LOG(LogTemp, Log, TEXT("PoliceAIController: Ending investigation"));
	InvestigationTarget = nullptr;
	InvestigationTimer = 0.0f;
	StartPatrolRoute();
}

void APoliceAIController::GoToMeetingPoint()
{
	if (!PatrolManager)
		return;

	APatrolWaypoint* MeetingPoint = PatrolManager->GetMeetingPointForRoute(AssignedRouteIndex);
	if (MeetingPoint)
	{
		CurrentState = EPoliceState::WaitingAtMeeting;

		FAIMoveRequest MoveRequest;
		MoveRequest.SetGoalLocation(MeetingPoint->GetActorLocation());
		MoveRequest.SetAcceptanceRadius(100.0f);
		MoveRequest.SetUsePathfinding(true);

		FNavPathSharedPtr NavPath;
		MoveTo(MoveRequest, &NavPath);

		UE_LOG(LogTemp, Log, TEXT("PoliceAIController: Going to meeting point"));

		GetWorldTimerManager().SetTimer(
			MeetingTimeoutTimer,
			this,
			&APoliceAIController::WaitAtMeetingPoint,
			1.0f,
			false,
			1.0f
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PoliceAIController: No meeting point found for route %d"), AssignedRouteIndex);
		StartPatrolRoute();
	}
}

void APoliceAIController::WaitAtMeetingPoint()
{
	if (!PatrolManager)
		return;

	PatrolManager->NotifyOfficerAtMeetingPoint(this, AssignedRouteIndex);
	bWaitingForPartner = true;
	MeetingWaitTimer = 0.0f;

	APatrolWaypoint* MeetingPoint = PatrolManager->GetMeetingPointForRoute(AssignedRouteIndex);
	if (MeetingPoint && MeetingPoint->bCoordinateWithOtherRoutes && MeetingPoint->ConnectedRoutes.Num() > 1)
	{
		bool bAllPartnersPresent = true;

		for (int32 RouteIndex : MeetingPoint->ConnectedRoutes)
		{
			if (RouteIndex == AssignedRouteIndex)
				continue;

			bool bPartnerFound = false;
			TArray<AActor*> AllPawns;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), AllPawns);

			for (AActor* PawnActor : AllPawns)
			{
				APawn* OfficerPawn = Cast<APawn>(PawnActor);
				if (!OfficerPawn || OfficerPawn == GetPawn())
					continue;

				APoliceAIController* OtherController = Cast<APoliceAIController>(OfficerPawn->GetController());
				if (OtherController && OtherController->GetAssignedRouteIndex() == RouteIndex)
				{
					if (OtherController->CurrentState == EPoliceState::WaitingAtMeeting)
					{
						bPartnerFound = true;
						break;
					}
				}
			}

			if (!bPartnerFound)
			{
				bAllPartnersPresent = false;
				break;
			}
		}

		if (bAllPartnersPresent)
		{
			UE_LOG(LogTemp, Log, TEXT("PoliceAIController: All partners present, performing swap"));
			PatrolManager->NotifyOfficerLeftMeetingPoint(this);
			PerformSwap(MeetingPoint->ConnectedRoutes);
			return;
		}
	}

	GetWorldTimerManager().SetTimer(
		MeetingTimeoutTimer,
		this,
		&APoliceAIController::OnMeetingTimeout,
		PatrolManager->MeetingWaitTimeout,
		false
	);
}

void APoliceAIController::OnMeetingTimeout()
{
	if (bWaitingForPartner)
	{
		UE_LOG(LogTemp, Log, TEXT("PoliceAIController: Meeting timeout, searching for partner"));
		SearchForPartner();
	}
}

void APoliceAIController::SearchForPartner()
{
	if (!PatrolManager)
		return;

	CurrentState = EPoliceState::SearchingPartner;

	APatrolWaypoint* MeetingPoint = PatrolManager->GetMeetingPointForRoute(AssignedRouteIndex);
	if (MeetingPoint && MeetingPoint->bCoordinateWithOtherRoutes && MeetingPoint->ConnectedRoutes.Num() > 1)
	{
		int32 PartnerRouteIndex = PatrolManager->FindPartnerRouteIndex(AssignedRouteIndex, MeetingPoint->ConnectedRoutes);
		TArray<APatrolWaypoint*> PartnerWaypoints = PatrolManager->GetRouteWaypoints(PartnerRouteIndex);

		if (PartnerWaypoints.Num() > 0)
		{
			int32 RandomIndex = FMath::RandRange(0, PartnerWaypoints.Num() - 1);

			FAIMoveRequest MoveRequest;
			MoveRequest.SetGoalLocation(PartnerWaypoints[RandomIndex]->GetActorLocation());
			MoveRequest.SetAcceptanceRadius(100.0f);
			MoveRequest.SetUsePathfinding(true);

			FNavPathSharedPtr NavPath;
			MoveTo(MoveRequest, &NavPath);

			GetWorldTimerManager().SetTimer(
				WaypointWaitTimer,
				[this, MeetingPoint]()
				{
					PatrolManager->NotifyOfficerLeftMeetingPoint(this);
					PerformSwap(MeetingPoint->ConnectedRoutes);
				},
				5.0f,
				false
			);
			return;
		}
	}

	PatrolManager->NotifyOfficerLeftMeetingPoint(this);
	TArray<int32> SingleRoute;
	SingleRoute.Add(AssignedRouteIndex);
	PerformSwap(SingleRoute);
}