#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "PatrolWaypoint.generated.h"

UCLASS()
class ASALTOPROJECT_API APatrolWaypoint : public AActor
{
	GENERATED_BODY()
	
public:	
	APatrolWaypoint();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Route")
	int32 RouteIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Route")
	int32 WaypointIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Route", meta = (EditCondition = "WaypointIndex == 0", EditConditionHides, Tooltip = "Assign NPCs that will patrol this route (multiple officers supported)"))
	TArray<TObjectPtr<APawn>> AssignedOfficers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Meeting Point")
	bool bIsMeetingPoint = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Meeting Point", meta = (EditCondition = "bIsMeetingPoint", EditConditionHides, Tooltip = "Should officers from different routes meet and swap here?"))
	bool bCoordinateWithOtherRoutes = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Meeting Point", meta = (EditCondition = "bIsMeetingPoint && bCoordinateWithOtherRoutes", EditConditionHides, Tooltip = "Which routes meet here (e.g., [0, 1] for routes 0 and 1)"))
	TArray<int32> ConnectedRoutes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Behavior")
	float WaitTime = 2.0f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	class UBillboardComponent* BillboardComp;

	UPROPERTY(VisibleAnywhere)
	class USphereComponent* SphereComp;
};
