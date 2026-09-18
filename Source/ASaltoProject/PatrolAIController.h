#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"

UENUM(BlueprintType)
enum class EAIState : uint8
{
	Patrolling,
	Chasing
};

#include "PatrolAIController.generated.h"

UCLASS()
class ASALTOPROJECT_API APatrolAIController : public AAIController
{
	GENERATED_BODY()

public:
	APatrolAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

private:
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void StartPatrol();
	void MoveToRandomLocation();
	void ChasePlayer(APawn* Player);

	UPROPERTY(EditAnywhere, Category = "AI|Patrol")
	float PatrolRadius = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Patrol")
	float PatrolWaitTime = 2.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Chase")
	float ChaseUpdateInterval = 0.5f;

	UPROPERTY(EditAnywhere, Category = "AI|Perception")
	float SightRadius = 1500.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Perception")
	float LoseSightRadius = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Perception")
	float PeripheralVisionAngleDegrees = 90.0f;

	UPROPERTY(VisibleAnywhere, Category = "AI|State")
	EAIState CurrentState;

	UPROPERTY()
	class UAIPerceptionComponent* AIPerceptionComp;

	UPROPERTY()
	class UAISenseConfig_Sight* SightConfig;

	UPROPERTY()
	APawn* TargetPlayer;

	FVector PatrolOrigin;
	float TimeSinceLastChaseUpdate;
	FTimerHandle PatrolTimerHandle;
};
