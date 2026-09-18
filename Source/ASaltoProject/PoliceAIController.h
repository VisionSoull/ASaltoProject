#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "PatrolWaypoint.h"

UENUM(BlueprintType)
enum class EPoliceState : uint8
{
	Patrolling,
	WaitingAtWaypoint,
	WaitingAtMeeting,
	Investigating,
	SearchingPartner
};

#include "PoliceAIController.generated.h"

UCLASS()
class ASALTOPROJECT_API APoliceAIController : public AAIController
{
	GENERATED_BODY()

public:
	APoliceAIController();

	void InitializeFromManager(class APolicePatrolManager* Manager, int32 RouteIndex, bool bClockwise);

	int32 GetAssignedRouteIndex() const { return AssignedRouteIndex; }
	bool IsPatrollingClockwise() const { return bPatrolClockwise; }

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

private:
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void StartPatrolRoute();
	void MoveToNextWaypoint();
	void OnWaypointReached();
	void PerformSwap(const TArray<int32>& ConnectedRoutes);

	void StartInvestigation(APawn* Target);
	void UpdateInvestigation(float DeltaTime);
	void EndInvestigation();

	void GoToMeetingPoint();
	void WaitAtMeetingPoint();
	void OnMeetingTimeout();
	void SearchForPartner();

	int32 GetNextWaypointIndex() const;

	UPROPERTY()
	class APolicePatrolManager* PatrolManager;

	UPROPERTY(EditAnywhere, Category = "AI|Perception")
	float SightRadius = 1500.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Perception")
	float LoseSightRadius = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Perception")
	float PeripheralVisionAngleDegrees = 90.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Investigation")
	float InvestigationUpdateInterval = 0.5f;

	UPROPERTY(EditAnywhere, Category = "AI|Investigation")
	float InvestigationTimeout = 10.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Waypoint")
	float WaypointReachedDistance = 100.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Avoidance")
	bool bEnableAvoidance = true;

	UPROPERTY(EditAnywhere, Category = "AI|Avoidance", meta = (EditCondition = "bEnableAvoidance"))
	float AvoidanceWeight = 0.5f;

	UPROPERTY(VisibleAnywhere, Category = "AI|State")
	EPoliceState CurrentState;

	UPROPERTY(VisibleAnywhere, Category = "AI|State")
	int32 AssignedRouteIndex;

	UPROPERTY(VisibleAnywhere, Category = "AI|State")
	bool bPatrolClockwise;

	UPROPERTY()
	class UAIPerceptionComponent* AIPerceptionComp;

	UPROPERTY()
	class UAISenseConfig_Sight* SightConfig;

	UPROPERTY()
	TArray<APatrolWaypoint*> CurrentWaypoints;

	UPROPERTY()
	APawn* InvestigationTarget;

	int32 CurrentWaypointIndex;
	int32 CompletedPatrolCount;
	float TimeSinceLastUpdate;
	float InvestigationTimer;
	float MeetingWaitTimer;
	bool bWaitingForPartner;
	bool bInitialized;

	FTimerHandle WaypointWaitTimer;
	FTimerHandle MeetingTimeoutTimer;

	FVector LastKnownTargetLocation;
};