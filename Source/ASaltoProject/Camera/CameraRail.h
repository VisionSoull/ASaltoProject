#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "CineCameraComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Curves/CurveFloat.h"
#include "CameraRail.generated.h"

UENUM(BlueprintType)
enum class ECameraSmoothingMode : uint8
{
    Off UMETA(DisplayName = "Off (Instant)"),
    LinearInterpolation UMETA(DisplayName = "Linear Interpolation (Lerp)"),
    Damping UMETA(DisplayName = "Damping/SmoothDamp")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRailPositionChanged, float, NewPosition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRailEndReached);

UCLASS(Blueprintable, BlueprintType)
class ASALTOPROJECT_API ACameraRail : public AActor
{
    GENERATED_BODY()

public:
    ACameraRail();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void BecomeViewTarget(APlayerController* PC) override;
    virtual void EndViewTarget(APlayerController* PC) override;

#if WITH_EDITOR
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
#endif

    // ==================================================================================
    // CORE COMPONENTS
    // ==================================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USceneComponent* RootSceneComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USplineComponent* SplineComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UCineCameraComponent* CameraComponent;

    // ==================================================================================
    // RAIL MOVEMENT
    // ==================================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail Movement")
    bool bLoopRail = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CurrentPosition = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float InitialRailPosition = 0.0f;

    // ==================================================================================
    // CAMERA POSITIONING
    // ==================================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Positioning")
    FVector CameraOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Positioning")
    FRotator CameraRotationOffset = FRotator::ZeroRotator;

    // Minimum Distance - Spline Movement
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Positioning|Minimum Distance")
    bool bEnforceMinimumDistanceOnSpline = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Positioning|Minimum Distance", 
        meta = (EditCondition = "bEnforceMinimumDistanceOnSpline", EditConditionHides, ClampMin = "0.0"))
    float MinimumDistanceToPlayer = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Positioning|Minimum Distance",
        meta = (EditCondition = "bEnforceMinimumDistanceOnSpline", EditConditionHides, ClampMin = "0.0"))
    float SplineDistanceCorrectionSpeed = 2.0f;

    // Minimum Distance - Offset Based (XY only, preserves altitude)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Positioning|Minimum Distance")
    bool bEnforceMinimumDistanceWithOffset = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Positioning|Minimum Distance",
        meta = (EditCondition = "bEnforceMinimumDistanceWithOffset", EditConditionHides, ClampMin = "0.0"))
    float MinimumOffsetDistance = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Positioning|Minimum Distance",
        meta = (EditCondition = "bEnforceMinimumDistanceWithOffset", EditConditionHides, ClampMin = "0.0"))
    float MaximumOffsetDistance = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Positioning|Minimum Distance",
        meta = (EditCondition = "bEnforceMinimumDistanceWithOffset", EditConditionHides))
    bool bUseOffsetX = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Positioning|Minimum Distance",
        meta = (EditCondition = "bEnforceMinimumDistanceWithOffset", EditConditionHides))
    bool bUseOffsetY = true;

    // ==================================================================================
    // CAMERA ROTATION
    // ==================================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation")
    bool bUseIndependentRotationSmoothing = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation",
        meta = (EditCondition = "bUseIndependentRotationSmoothing", EditConditionHides, ClampMin = "0.0"))
    float RotationLerpSpeed = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation", meta = (ClampMin = "0.0"))
    float CentralizationSpeed = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation", meta = (ClampMin = "0.0"))
    float PlayerStoppedThreshold = 50.0f;

    // Axis Constraints
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation|Axis Constraints")
    bool bLockPitch = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation|Axis Constraints", 
        meta = (EditCondition = "bLockPitch", EditConditionHides))
    float LockedPitchValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation|Axis Constraints")
    bool bLockYaw = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation|Axis Constraints", 
        meta = (EditCondition = "bLockYaw", EditConditionHides))
    float LockedYawValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation|Axis Constraints")
    bool bLockRoll = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation|Axis Constraints", 
        meta = (EditCondition = "bLockRoll", EditConditionHides))
    float LockedRollValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation|Axis Constraints")
    bool bConstrainPitchRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation|Axis Constraints", 
        meta = (EditCondition = "bConstrainPitchRange", EditConditionHides, ClampMin = "-90.0", ClampMax = "90.0"))
    float MinPitch = -45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation|Axis Constraints", 
        meta = (EditCondition = "bConstrainPitchRange", EditConditionHides, ClampMin = "-90.0", ClampMax = "90.0"))
    float MaxPitch = 45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation|Axis Constraints")
    bool bConstrainYawRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation|Axis Constraints", 
        meta = (EditCondition = "bConstrainYawRange", EditConditionHides, ClampMin = "-180.0", ClampMax = "180.0"))
    float MinYaw = -90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation|Axis Constraints", 
        meta = (EditCondition = "bConstrainYawRange", EditConditionHides, ClampMin = "-180.0", ClampMax = "180.0"))
    float MaxYaw = 90.0f;

    // ==================================================================================
    // CINEMATIC CAMERA SETTINGS
    // ==================================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
    bool bEnableDepthOfField = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera",
        meta = (EditCondition = "bEnableDepthOfField", EditConditionHides, ClampMin = "1.0", ClampMax = "32.0"))
    float DepthOfFieldAperture = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
    bool bEnableFocusTracking = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera",
        meta = (EditCondition = "bEnableFocusTracking", EditConditionHides))
    FVector FocusTrackingOffset = FVector::ZeroVector;

    // ==================================================================================
    // SMOOTHING
    // ==================================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoothing")
    ECameraSmoothingMode SmoothingMode = ECameraSmoothingMode::LinearInterpolation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoothing", 
        meta = (EditCondition = "SmoothingMode == ECameraSmoothingMode::LinearInterpolation", EditConditionHides, ClampMin = "0.0"))
    float LerpSpeed = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoothing", 
        meta = (EditCondition = "SmoothingMode == ECameraSmoothingMode::Damping", EditConditionHides, ClampMin = "0.0"))
    float DampingSpeed = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoothing", 
        meta = (EditCondition = "SmoothingMode == ECameraSmoothingMode::Damping", EditConditionHides, ClampMin = "0.0"))
    float MaxDampingSpeed = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoothing")
    bool bUseCameraSpeedLimit = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoothing", 
        meta = (EditCondition = "bUseCameraSpeedLimit", EditConditionHides, ClampMin = "0.0"))
    float MaxCameraSpeed = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoothing")
    bool bUseSpeedCurve = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoothing", 
        meta = (EditCondition = "bUseSpeedCurve", EditConditionHides))
    UCurveFloat* SpeedCurve = nullptr;

    // ==================================================================================
    // CAMERA SHAKE
    // ==================================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
    bool bEnableCameraShake = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake", 
        meta = (EditCondition = "bEnableCameraShake", EditConditionHides))
    TSubclassOf<UCameraShakeBase> CameraShakeClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake", 
        meta = (EditCondition = "bEnableCameraShake", EditConditionHides))
    float ShakeScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake", 
        meta = (EditCondition = "bEnableCameraShake", EditConditionHides))
    bool bUsePlayerSpeed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake", 
        meta = (EditCondition = "bEnableCameraShake && bUsePlayerSpeed", EditConditionHides, ClampMin = "0.0"))
    float MinSpeedForShake = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake", 
        meta = (EditCondition = "bEnableCameraShake && bUsePlayerSpeed", EditConditionHides))
    bool bScaleShakeWithSpeed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake", 
        meta = (EditCondition = "bEnableCameraShake && bUsePlayerSpeed && bScaleShakeWithSpeed", EditConditionHides))
    float MaxSpeedForMaxShake = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake|Idle Shake", 
        meta = (EditCondition = "bEnableCameraShake", EditConditionHides))
    bool bEnableIdleShake = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake|Idle Shake", 
        meta = (EditCondition = "bEnableCameraShake && bEnableIdleShake", EditConditionHides))
    TSubclassOf<UCameraShakeBase> IdleShakeClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake|Idle Shake", 
        meta = (EditCondition = "bEnableCameraShake && bEnableIdleShake", EditConditionHides))
    float IdleShakeScale = 0.3f;

    // ==================================================================================
    // PLAYER REFERENCE & AUTO ACTIVATION
    // ==================================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Rail")
    AActor* PlayerReference = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rail")
    bool bAutoActivateOnPlay = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rail")
    bool bAutoDeactivateWhenNotViewTarget = true;

    // ==================================================================================
    // DEBUG
    // ==================================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebugInfo = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bDrawSpline = true;

    // ==================================================================================
    // EVENTS
    // ==================================================================================

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnRailPositionChanged OnRailPositionChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnRailEndReached OnRailEndReached;

private:
    // ==================================================================================
    // CONSTANTS
    // ==================================================================================

    static constexpr float PlayerSearchInterval = 0.2f;
    static constexpr int32 MaxPlayerSearchAttempts = 100;
    static constexpr int32 PlayerSearchLogInterval = 10;
    static constexpr float PlayerMovementThresholdSquared = 100.0f;
    static constexpr float DebugCameraSphereRadius = 50.0f;
    static constexpr float DebugPlayerSphereRadius = 30.0f;
    static constexpr float DebugDirectionArrowLength = 200.0f;
    static constexpr float DebugMinDistanceSphereRadius = 25.0f;

    // ==================================================================================
    // INTERNAL STATE
    // ==================================================================================

    float TargetPosition = 0.0f;
    bool bHasReachedEnd = false;
    bool bIsActiveViewTarget = false;

    FVector LastPlayerLocation = FVector::ZeroVector;
    bool bPlayerLocationInitialized = false;

    FTimerHandle PlayerSearchTimer;
    int32 PlayerSearchAttempts = 0;

    float DampingVelocity = 0.0f;

    // ==================================================================================
    // CAMERA SHAKE STATE
    // ==================================================================================

    struct FCameraShakeState
    {
        UCameraShakeBase* ActiveMovementShake = nullptr;
        UCameraShakeBase* ActiveIdleShake = nullptr;
        FVector LastPlayerLocation = FVector::ZeroVector;
        float CurrentPlayerSpeed = 0.0f;
        bool bIsPlayerMoving = false;

        void Reset()
        {
            ActiveMovementShake = nullptr;
            ActiveIdleShake = nullptr;
            LastPlayerLocation = FVector::ZeroVector;
            CurrentPlayerSpeed = 0.0f;
            bIsPlayerMoving = false;
        }
    };

    FCameraShakeState ShakeState;

    void UpdateCameraShake(float DeltaTime);
    float CalculatePlayerSpeed(float DeltaTime);
    void StartMovementShake(float Intensity);
    void StartIdleShake();
    void StopMovementShake(bool bImmediate = false);
    void StopIdleShake(bool bImmediate = false);
    void StopAllShakes(bool bImmediate = true);

    void UpdateCameraFocusSettings();

public:
    // ==================================================================================
    // PUBLIC INTERFACE
    // ==================================================================================

    UFUNCTION(BlueprintCallable, Category = "Camera Rail")
    float GetRailPosition() const { return CurrentPosition; }

    UFUNCTION(BlueprintCallable, Category = "Camera Rail")
    UCineCameraComponent* GetCameraComponent() const { return CameraComponent; }

    UFUNCTION(BlueprintCallable, Category = "Camera Rail")
    USplineComponent* GetSplineComponent() const { return SplineComponent; }

    UFUNCTION(BlueprintCallable, Category = "Camera Rail")
    void SetRailPosition(float Position, bool bTeleport = false);

    UFUNCTION(BlueprintCallable, Category = "Camera Rail")
    void SetPlayerReference(AActor* NewPlayer);

    UFUNCTION(BlueprintCallable, Category = "Camera Rail")
    float GetRailLength() const;

    UFUNCTION(BlueprintCallable, Category = "Camera Rail")
    FVector GetLocationAtPosition(float Position) const;

    UFUNCTION(BlueprintCallable, Category = "Camera Rail")
    FRotator GetRotationAtPosition(float Position) const;

    UFUNCTION(BlueprintCallable, Category = "Camera Rail")
    float FindClosestPositionToLocation(const FVector& WorldLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Camera Rail")
    void ActivateCamera();

    UFUNCTION(BlueprintCallable, Category = "Camera Rail")
    void DeactivateCamera();

    UFUNCTION(BlueprintCallable, Category = "Camera Rail|Camera Shake")
    float GetCurrentPlayerSpeed() const { return ShakeState.CurrentPlayerSpeed; }

    UFUNCTION(BlueprintCallable, Category = "Camera Rail|Camera Shake")
    bool IsPlayerMoving() const { return ShakeState.bIsPlayerMoving; }

    UFUNCTION(BlueprintCallable, Category = "Camera Rail|Cinematic Camera")
    void SetDepthOfFieldEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Camera Rail|Cinematic Camera")
    void SetFocusTrackingEnabled(bool bEnabled);

private:
    // ==================================================================================
    // INTERNAL FUNCTIONS
    // ==================================================================================

    void UpdateCameraPosition(float DeltaTime);
    void UpdateCameraRotation(float DeltaTime);
    void HandlePlayerFollowing(float DeltaTime);
    void CheckViewTargetStatus();
    void DrawDebugInfo();
    float GetSpeedMultiplierAtPosition(float Position) const;
    void FindAndSetPlayer();

    FRotator CalculateDesiredCameraRotation() const;
    bool IsPlayerStopped() const;
    
    FORCEINLINE float GetCurrentDistanceAlongSpline() const
    {
        return CurrentPosition * GetRailLength();
    }

    void ApplySplineDistanceConstraint(float& InOutTargetPosition, const FVector& PlayerLocation);
    void ApplyOffsetDistanceConstraint(FVector& InOutCameraLocation, const FVector& PlayerLocation, const FRotator& SplineRotation);
};