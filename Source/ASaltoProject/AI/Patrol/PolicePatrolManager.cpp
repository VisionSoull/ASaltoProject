#include "PolicePatrolManager.h"
#include "PoliceAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

APolicePatrolManager::APolicePatrolManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APolicePatrolManager::BeginPlay()
{
	Super::BeginPlay();
	GatherWaypoints();
	DiscoverOfficerAssignments();
	InitializeOfficers();
}

void APolicePatrolManager::GatherWaypoints()
{
	TArray<AActor*> FoundWaypoints;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APatrolWaypoint::StaticClass(), FoundWaypoints);

	for (AActor* Actor : FoundWaypoints)
	{
		APatrolWaypoint* Waypoint = Cast<APatrolWaypoint>(Actor);
		if (!Waypoint)
			continue;

		if (Waypoint->bIsMeetingPoint && Waypoint->bCoordinateWithOtherRoutes)
		{
			FMeetingPointInfo MeetingInfo;
			MeetingInfo.MeetingWaypoint = Waypoint;
			MeetingInfo.ConnectedRoutes = Waypoint->ConnectedRoutes;
			MeetingPoints.Add(MeetingInfo);
		}

		if (!RouteWaypoints.Contains(Waypoint->RouteIndex))
		{
			RouteWaypoints.Add(Waypoint->RouteIndex, TArray<APatrolWaypoint*>());
		}
		RouteWaypoints[Waypoint->RouteIndex].Add(Waypoint);
	}

	for (auto& RoutePair : RouteWaypoints)
	{
		RoutePair.Value.Sort([](const APatrolWaypoint& A, const APatrolWaypoint& B) {
			return A.WaypointIndex < B.WaypointIndex;
			});
	}

	UE_LOG(LogTemp, Log, TEXT("PolicePatrolManager: Found %d routes and %d meeting points"), RouteWaypoints.Num(), MeetingPoints.Num());
}

void APolicePatrolManager::DiscoverOfficerAssignments()
{
	for (auto& RoutePair : RouteWaypoints)
	{
		int32 RouteIndex = RoutePair.Key;
		TArray<APatrolWaypoint*>& Waypoints = RoutePair.Value;

		for (APatrolWaypoint* Waypoint : Waypoints)
		{
			if (Waypoint->AssignedOfficers.Num() > 0)
			{
				int32 OfficerIndex = 0;
				for (APawn* Officer : Waypoint->AssignedOfficers)
				{
					if (!Officer)
						continue;

					bool bAlreadyAssigned = false;
					for (const FOfficerRouteAssignment& Assignment : OfficerAssignments)
					{
						if (Assignment.OfficerPawn == Officer)
						{
							bAlreadyAssigned = true;
							break;
						}
					}

					if (!bAlreadyAssigned)
					{
						FOfficerRouteAssignment NewAssignment;
						NewAssignment.OfficerPawn = Officer;
						NewAssignment.RouteIndex = RouteIndex;
						NewAssignment.bStartClockwise = (OfficerIndex % 2 == 0);
						OfficerAssignments.Add(NewAssignment);

						UE_LOG(LogTemp, Log, TEXT("PolicePatrolManager: Discovered officer %s assigned to route %d (%s)"),
							*Officer->GetName(), RouteIndex,
							NewAssignment.bStartClockwise ? TEXT("Clockwise") : TEXT("Counter-clockwise"));
					}
					OfficerIndex++;
				}
				break;
			}
		}
	}
}

void APolicePatrolManager::InitializeOfficers()
{
	for (FOfficerRouteAssignment& Assignment : OfficerAssignments)
	{
		if (!Assignment.OfficerPawn)
			continue;

		APawn* OfficerPawn = Cast<APawn>(Assignment.OfficerPawn);
		if (!OfficerPawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("PolicePatrolManager: Assigned officer is not a Pawn!"));
			continue;
		}

		APoliceAIController* AIController = Cast<APoliceAIController>(OfficerPawn->GetController());
		if (!AIController)
		{
			UE_LOG(LogTemp, Warning, TEXT("PolicePatrolManager: Officer %s does not have PoliceAIController!"),
				*OfficerPawn->GetName());
			continue;
		}

		Assignment.OfficerController = AIController;
		AIController->InitializeFromManager(this, Assignment.RouteIndex, Assignment.bStartClockwise);

		UE_LOG(LogTemp, Log, TEXT("PolicePatrolManager: Initialized officer %s on route %d"),
			*OfficerPawn->GetName(), Assignment.RouteIndex);
	}
}

void APolicePatrolManager::RegisterOfficerFromWaypoint(AActor* OfficerPawn, int32 RouteIndex)
{
	if (!OfficerPawn)
		return;

	for (const FOfficerRouteAssignment& Assignment : OfficerAssignments)
	{
		if (Assignment.OfficerPawn == OfficerPawn)
			return;
	}

	FOfficerRouteAssignment NewAssignment;
	NewAssignment.OfficerPawn = OfficerPawn;
	NewAssignment.RouteIndex = RouteIndex;
	NewAssignment.bStartClockwise = true;
	OfficerAssignments.Add(NewAssignment);
}

APoliceAIController* APolicePatrolManager::GetOfficerController(AActor* OfficerPawn)
{
	for (const FOfficerRouteAssignment& Assignment : OfficerAssignments)
	{
		if (Assignment.OfficerPawn == OfficerPawn)
		{
			return Assignment.OfficerController;
		}
	}
	return nullptr;
}

int32 APolicePatrolManager::GetOfficerRouteIndex(APoliceAIController* Officer) const
{
	for (const FOfficerRouteAssignment& Assignment : OfficerAssignments)
	{
		if (Assignment.OfficerController == Officer)
		{
			return Assignment.RouteIndex;
		}
	}
	return 0;
}

TArray<APatrolWaypoint*> APolicePatrolManager::GetRouteWaypoints(int32 RouteIndex) const
{
	if (RouteWaypoints.Contains(RouteIndex))
	{
		return RouteWaypoints[RouteIndex];
	}
	return TArray<APatrolWaypoint*>();
}

APatrolWaypoint* APolicePatrolManager::GetMeetingPointForRoute(int32 RouteIndex) const
{
	for (const FMeetingPointInfo& MeetingInfo : MeetingPoints)
	{
		if (MeetingInfo.ConnectedRoutes.Contains(RouteIndex))
		{
			return MeetingInfo.MeetingWaypoint;
		}
	}

	for (auto& RoutePair : RouteWaypoints)
	{
		if (RoutePair.Key == RouteIndex)
		{
			for (APatrolWaypoint* Waypoint : RoutePair.Value)
			{
				if (Waypoint->bIsMeetingPoint && !Waypoint->bCoordinateWithOtherRoutes)
				{
					return Waypoint;
				}
			}
		}
	}

	return nullptr;
}

bool APolicePatrolManager::ShouldMeetAfterPatrol(int32 CompletedPatrols) const
{
	return CompletedPatrols > 0 && CompletedPatrols % PatrolsBeforeMeeting == 0;
}

void APolicePatrolManager::NotifyOfficerAtMeetingPoint(APoliceAIController* Officer, int32 RouteIndex)
{
	if (!Officer)
		return;

	for (FMeetingPointInfo& MeetingInfo : MeetingPoints)
	{
		if (MeetingInfo.ConnectedRoutes.Contains(RouteIndex))
		{
			if (!MeetingInfo.WaitingOfficers.Contains(Officer))
			{
				MeetingInfo.WaitingOfficers.Add(Officer);
				UE_LOG(LogTemp, Log, TEXT("Officer arrived at meeting point. Total waiting: %d"), MeetingInfo.WaitingOfficers.Num());
			}
			break;
		}
	}
}

void APolicePatrolManager::NotifyOfficerLeftMeetingPoint(APoliceAIController* Officer)
{
	for (FMeetingPointInfo& MeetingInfo : MeetingPoints)
	{
		MeetingInfo.WaitingOfficers.Remove(Officer);
	}
}

int32 APolicePatrolManager::FindPartnerRouteIndex(int32 CurrentRouteIndex, const TArray<int32>& ConnectedRoutes) const
{
	for (int32 RouteIndex : ConnectedRoutes)
	{
		if (RouteIndex != CurrentRouteIndex)
		{
			return RouteIndex;
		}
	}
	return CurrentRouteIndex;
}