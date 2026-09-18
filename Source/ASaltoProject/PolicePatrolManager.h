#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PatrolWaypoint.h"

UENUM(BlueprintType)
enum class ESwapMode : uint8
{
	SwapRoutes UMETA(DisplayName = "Swap Routes (Officers exchange patrol areas)"),
	SwapDirection UMETA(DisplayName = "Swap Direction (Officers reverse on same route)")
};

#include "PolicePatrolManager.generated.h"

USTRUCT()
struct FMeetingPointInfo
{
	GENERATED_BODY()

	UPROPERTY()
	APatrolWaypoint* MeetingWaypoint = nullptr;

	UPROPERTY()
	TArray<int32> ConnectedRoutes;

	UPROPERTY()
	TArray<class APoliceAIController*> WaitingOfficers;
};

USTRUCT()
struct FOfficerRouteAssignment
{
	GENERATED_BODY()

	UPROPERTY()
	AActor* OfficerPawn = nullptr;

	UPROPERTY()
	class APoliceAIController* OfficerController = nullptr;

	int32 RouteIndex = 0;
	bool bStartClockwise = true;
};

UCLASS()
class ASALTOPROJECT_API APolicePatrolManager : public AActor
{
	GENERATED_BODY()
	
public:	
	APolicePatrolManager();

	void RegisterOfficerFromWaypoint(AActor* OfficerPawn, int32 RouteIndex);
	class APoliceAIController* GetOfficerController(AActor* OfficerPawn);
	
	TArray<APatrolWaypoint*> GetRouteWaypoints(int32 RouteIndex) const;
	APatrolWaypoint* GetMeetingPointForRoute(int32 RouteIndex) const;
	
	bool ShouldMeetAfterPatrol(int32 CompletedPatrols) const;
	
	void NotifyOfficerAtMeetingPoint(class APoliceAIController* Officer, int32 RouteIndex);
	void NotifyOfficerLeftMeetingPoint(class APoliceAIController* Officer);

	int32 FindPartnerRouteIndex(int32 CurrentRouteIndex, const TArray<int32>& ConnectedRoutes) const;
	
	int32 GetOfficerRouteIndex(class APoliceAIController* Officer) const;

	UPROPERTY(EditAnywhere, Category = "Patrol")
	ESwapMode SwapMode = ESwapMode::SwapRoutes;

	UPROPERTY(EditAnywhere, Category = "Patrol")
	int32 PatrolsBeforeMeeting = 3;

	UPROPERTY(EditAnywhere, Category = "Patrol")
	float MeetingWaitTimeout = 30.0f;

protected:
	virtual void BeginPlay() override;

private:
	void GatherWaypoints();
	void DiscoverOfficerAssignments();
	void InitializeOfficers();

	TMap<int32, TArray<APatrolWaypoint*>> RouteWaypoints;
	TArray<FMeetingPointInfo> MeetingPoints;
	TArray<FOfficerRouteAssignment> OfficerAssignments;
};
