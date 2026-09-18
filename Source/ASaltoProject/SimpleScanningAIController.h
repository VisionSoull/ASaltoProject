#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SimpleScanningAIController.generated.h"

UENUM(BlueprintType)
enum class EScanningAIState : uint8
{
	Idle UMETA(DisplayName = "Idle - Scanning at spawn location"),
	Chasing UMETA(DisplayName = "Chasing - Moving toward player"),
	Interacting UMETA(DisplayName = "Interacting - Stopped near player"),
	Returning UMETA(DisplayName = "Returning - Going back to spawn")
};

UCLASS()
class ASALTOPROJECT_API ASimpleScanningAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASimpleScanningAIController();

	// Called from Blueprint to signal interaction is complete
	UFUNCTION(BlueprintCallable, Category = "AI")
	void CompleteInteraction();

	UFUNCTION(BlueprintPure, Category = "AI")
	EScanningAIState GetCurrentState() const { return CurrentState; }

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

	// Blueprint event triggered when AI reaches interaction distance with player
	UFUNCTION(BlueprintImplementableEvent, Category = "AI", meta = (DisplayName = "On Player Reached"))
	void OnPlayerReached(APawn* Player);

private:
	UFUNCTION()
	void OnSeePawn(APawn* SeenPawn);

	void StartChasing(APawn* Target);
	void StartReturning();
	void CheckInteractionDistance();
	void UpdateScanning(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = "AI|Detection")
	float SightRadius = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Detection")
	float HearingThreshold = 600.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Detection")
	float PeripheralVisionAngle = 90.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Chase")
	float InteractionDistance = 150.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Chase")
	float ChaseAcceptanceRadius = 50.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Chase")
	float LoseTargetDistance = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Return")
	float ReturnAcceptanceRadius = 100.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Avoidance")
	bool bEnableAvoidance = true;

	UPROPERTY(EditAnywhere, Category = "AI|Avoidance", meta = (EditCondition = "bEnableAvoidance"))
	float AvoidanceWeight = 0.5f;

	UPROPERTY(EditAnywhere, Category = "AI|Scanning")
	float ScanRotationSpeed = 45.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Scanning")
	float ScanAngleRange = 90.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Scanning")
	float PauseDurationAtEdge = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|State", meta = (AllowPrivateAccess = "true"))
	EScanningAIState CurrentState;

	UPROPERTY()
	class UPawnSensingComponent* PawnSensingComp;

	UPROPERTY()
	APawn* TargetPlayer;

	FVector SpawnLocation;
	FRotator InitialRotation;
	float CurrentScanAngle;
	bool bScanningRight;
	float PauseTimer;
	bool bInteractionTriggered;
};