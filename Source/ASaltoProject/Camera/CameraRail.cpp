#include "CameraRail.h"
#include "Components/SplineComponent.h"
#include "CineCameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

ACameraRail::ACameraRail()
{
    // ==================================================================================
    // Constructor
    // - Spline and Cine Camera
    // - Attaches components and sets up a simple default spline with two points
    // ==================================================================================
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootSceneComponent;

    SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
    SplineComponent->SetupAttachment(RootComponent);
    SplineComponent->SetClosedLoop(false);

    CameraComponent = CreateDefaultSubobject<UCineCameraComponent>(TEXT("Camera"));
    CameraComponent->SetupAttachment(RootComponent);

    if (SplineComponent)
    {
        SplineComponent->ClearSplinePoints();
        SplineComponent->AddSplinePoint(FVector(0, 0, 0), ESplineCoordinateSpace::Local);
        SplineComponent->AddSplinePoint(FVector(500, 0, 0), ESplineCoordinateSpace::Local);
        SplineComponent->UpdateSpline();
    }
}

void ACameraRail::BeginPlay()
{
    // ==================================================================================
    // BeginPlay
    // - Ensures initial/current rail position is set
    // - Resets camera shake state
    // - Applies focus settings to the cine camera
    // - Starts a repeating timer to search and bind the player actor
    // ==================================================================================
    Super::BeginPlay();

    UWorld* World = GetWorld();
    if (!World)
        return;

    if (InitialRailPosition > 0.0f || CurrentPosition == 0.0f)
    {
        CurrentPosition = InitialRailPosition;
    }

    TargetPosition = CurrentPosition;
    ShakeState.Reset();

    UpdateCameraFocusSettings();

    UE_LOG(LogTemp, Log, TEXT("CameraRail [%s]: Initialized at position %.2f"), *GetName(), CurrentPosition);

    World->GetTimerManager().SetTimer(
        PlayerSearchTimer,
        this,
        &ACameraRail::FindAndSetPlayer,
        PlayerSearchInterval,
        true
    );
}

void ACameraRail::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // ==================================================================================
    // EndPlay
    // Clean up timers and active camera shakes when the actor is being removed or the
    // level is being unloaded. Ensures no lingering timers or shake references remain.
    // ==================================================================================
    Super::EndPlay(EndPlayReason);

    UWorld* World = GetWorld();
    if (!World)
        return;

    FTimerManager& TimerManager = World->GetTimerManager();

    if (TimerManager.IsTimerActive(PlayerSearchTimer))
    {
        TimerManager.ClearTimer(PlayerSearchTimer);
    }

    StopAllShakes(true);
}

void ACameraRail::Tick(float DeltaTime)
{
    // ==================================================================================
    // Tick
    // When the actor is the active view target and the world is running the game/PIE:
    // - Optionally auto-deactivate when no longer the view target
    // - Update player-following logic, camera position, rotation and shake
    // - Optionally draw debug information
    // In editor mode the spline debug drawing is also handled here.
    // ==================================================================================
    Super::Tick(DeltaTime);

    if (!SplineComponent || !CameraComponent)
        return;

    bool bIsPlaying = (GetWorld()->WorldType == EWorldType::Game ||
        GetWorld()->WorldType == EWorldType::PIE);

    if (bIsPlaying)
    {
        if (bAutoDeactivateWhenNotViewTarget)
        {
            CheckViewTargetStatus();
        }

        if (bIsActiveViewTarget)
        {
            HandlePlayerFollowing(DeltaTime);
            UpdateCameraPosition(DeltaTime);
            UpdateCameraRotation(DeltaTime);
            UpdateCameraShake(DeltaTime);

            if (bShowDebugInfo)
            {
                DrawDebugInfo();
            }
        }
    }

#if WITH_EDITOR
    if (bDrawSpline && GetWorld() && GetWorld()->WorldType == EWorldType::Editor)
    {
        SplineComponent->SetDrawDebug(true);
    }
#endif
}

#if WITH_EDITOR
void ACameraRail::OnConstruction(const FTransform& Transform)
{
    // ==================================================================================
    // OnConstruction
    // Updates spline debug visibility and camera focus settings to reflect
    // current property values immediately in the editor viewport.
    // ==================================================================================
    Super::OnConstruction(Transform);

    if (SplineComponent)
    {
        SplineComponent->SetDrawDebug(bDrawSpline);
    }

    UpdateCameraFocusSettings();
}
#endif


void ACameraRail::BecomeViewTarget(APlayerController* PC)
{
    // ==================================================================================
    // BecomeViewTarget
    // Called when this actor becomes the active camera view target for a PlayerController.
    // Marks the rail as active which enables position/rotation updates while ticking.
    // ==================================================================================
    Super::BecomeViewTarget(PC);

    bIsActiveViewTarget = true;

    UE_LOG(LogTemp, Log, TEXT("CameraRail [%s]: Became active view target"), *GetName());
}

void ACameraRail::EndViewTarget(APlayerController* PC)
{
    // ==================================================================================
    // EndViewTarget
    // Called when this actor is no longer the active view target.
    // Clears the active flag so updates and shakes are disabled until reactivated.
    // ==================================================================================
    Super::EndViewTarget(PC);

    bIsActiveViewTarget = false;

    UE_LOG(LogTemp, Log, TEXT("CameraRail [%s]: No longer view target"), *GetName());
}

void ACameraRail::UpdateCameraPosition(float DeltaTime)
{
    // ==================================================================================
    // UpdateCameraPosition
    // Moves the current camera position along the spline toward the target position.
    // Supports several smoothing modes (instant, lerp, damping), optional speed
    // limiting (normalized to spline length), looping, and clamps to rail bounds.
    // After computing the normalized position, it converts to world-space location
    // applying the configured offset and enforces minimum distance constraints when
    // required. Finally broadcasts the position change event.
    // ==================================================================================
    if (!SplineComponent || !CameraComponent)
        return;

    float RailLength = GetRailLength();
    if (RailLength <= 0.0f)
        return;

    float SpeedMultiplier = GetSpeedMultiplierAtPosition(CurrentPosition);

    if (bUseCameraSpeedLimit && DeltaTime > 0.0f)
    {
        float MaxPositionDelta = (MaxCameraSpeed * DeltaTime) / RailLength;
        float TargetDelta = TargetPosition - CurrentPosition;

        if (FMath::Abs(TargetDelta) > MaxPositionDelta)
        {
            float Direction = FMath::Sign(TargetDelta);
            TargetPosition = CurrentPosition + (Direction * MaxPositionDelta);
        }
    }

    switch (SmoothingMode)
    {
    case ECameraSmoothingMode::Off:
        CurrentPosition = TargetPosition;
        break;

    case ECameraSmoothingMode::LinearInterpolation:
        CurrentPosition = FMath::FInterpTo(CurrentPosition, TargetPosition, DeltaTime, LerpSpeed * SpeedMultiplier);
        break;

    case ECameraSmoothingMode::Damping:
    {
        float Distance = TargetPosition - CurrentPosition;
        float AdjustedSpeed = DampingSpeed * SpeedMultiplier;

        DampingVelocity += Distance * AdjustedSpeed * DeltaTime;
        DampingVelocity = FMath::Clamp(DampingVelocity, -MaxDampingSpeed, MaxDampingSpeed);
        DampingVelocity *= FMath::Exp(-AdjustedSpeed * DeltaTime);

        CurrentPosition += DampingVelocity * DeltaTime;
    }
    break;
    }

    if (!bLoopRail)
    {
        CurrentPosition = FMath::Clamp(CurrentPosition, 0.0f, 1.0f);

        if (CurrentPosition >= 1.0f || CurrentPosition <= 0.0f)
        {
            if (!bHasReachedEnd)
            {
                OnRailEndReached.Broadcast();
                bHasReachedEnd = true;
            }
        }
        else
        {
            bHasReachedEnd = false;
        }
    }
    else
    {
        while (CurrentPosition > 1.0f) CurrentPosition -= 1.0f;
        while (CurrentPosition < 0.0f) CurrentPosition += 1.0f;
    }

    float CurrentDistance = GetCurrentDistanceAlongSpline();
    FVector SplineLocation = SplineComponent->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World);
    FRotator SplineRotation = SplineComponent->GetRotationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World);

    FVector WorldOffset = SplineRotation.RotateVector(CameraOffset);
    FVector FinalCameraLocation = SplineLocation + WorldOffset;

    if (bEnforceMinimumDistanceWithOffset && PlayerReference && IsValid(PlayerReference))
    {
        ApplyOffsetDistanceConstraint(FinalCameraLocation, PlayerReference->GetActorLocation(), SplineRotation);
    }

    CameraComponent->SetWorldLocation(FinalCameraLocation);

    OnRailPositionChanged.Broadcast(CurrentPosition);
}

void ACameraRail::UpdateCameraRotation(float DeltaTime)
{
    // ==================================================================================
    // UpdateCameraRotation
    // Smoothly rotates the cine camera to look at the player reference. Uses either the
    // independent rotation smoothing speed or the main lerp speed depending on
    // configuration and whether the player is moving. Rotation offsets and axis locks
    // / constraints are applied via CalculateDesiredCameraRotation().
    // ==================================================================================
    if (!CameraComponent || !PlayerReference || !IsValid(PlayerReference))
        return;

    FRotator TargetRotation = CalculateDesiredCameraRotation();

    float EffectiveSpeed;
    if (bUseIndependentRotationSmoothing)
    {
        EffectiveSpeed = IsPlayerStopped() ? CentralizationSpeed : RotationLerpSpeed;
    }
    else
    {
        EffectiveSpeed = IsPlayerStopped() ? CentralizationSpeed : LerpSpeed;
    }

    FRotator NewRotation = FMath::RInterpTo(
        CameraComponent->GetComponentRotation(),
        TargetRotation,
        DeltaTime,
        EffectiveSpeed
    );

    CameraComponent->SetWorldRotation(NewRotation);
}

FRotator ACameraRail::CalculateDesiredCameraRotation() const
{
    // ==================================================================================
    // CalculateDesiredCameraRotation
    // Computes the desired world rotation for the camera so it faces the player.
    // Applies configured pitch/yaw/roll locks and constrains pitch/yaw to ranges if
    // enabled. Adds any manual rotation offset before returning the target rotation.
    // ==================================================================================
    if (!CameraComponent || !PlayerReference || !IsValid(PlayerReference))
        return CameraComponent ? CameraComponent->GetComponentRotation() : FRotator::ZeroRotator;

    FVector LookDirection = (PlayerReference->GetActorLocation() - CameraComponent->GetComponentLocation()).GetSafeNormal();
    FRotator TargetRotation = UKismetMathLibrary::MakeRotFromX(LookDirection);

    TargetRotation += CameraRotationOffset;

    if (bLockPitch)
    {
        TargetRotation.Pitch = LockedPitchValue;
    }
    else if (bConstrainPitchRange)
    {
        TargetRotation.Pitch = FMath::Clamp(TargetRotation.Pitch, MinPitch, MaxPitch);
    }

    if (bLockYaw)
    {
        TargetRotation.Yaw = LockedYawValue;
    }
    else if (bConstrainYawRange)
    {
        float NormalizedYaw = FMath::UnwindDegrees(TargetRotation.Yaw);
        float NormalizedMin = FMath::UnwindDegrees(MinYaw);
        float NormalizedMax = FMath::UnwindDegrees(MaxYaw);
        TargetRotation.Yaw = FMath::Clamp(NormalizedYaw, NormalizedMin, NormalizedMax);
    }

    if (bLockRoll)
    {
        TargetRotation.Roll = LockedRollValue;
    }

    return TargetRotation;
}

bool ACameraRail::IsPlayerStopped() const
{
    // ==================================================================================
    // IsPlayerStopped
    // Utility that returns true when the recorded player speed is below the
    // configured threshold used to switch rotation smoothing behavior.
    // ==================================================================================
    return ShakeState.CurrentPlayerSpeed < PlayerStoppedThreshold;
}

void ACameraRail::CheckViewTargetStatus()
{
    // ==================================================================================
    // CheckViewTargetStatus
    // Verifies whether this rail is currently the PlayerController's view target and
    // auto-activates or deactivates itself based on configuration. Also stops shakes
    // when auto-deactivating.
    // ==================================================================================
    UWorld* World = GetWorld();
    if (!World)
        return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
        return;

    AActor* CurrentViewTarget = PC->GetViewTarget();
    bool bShouldBeActive = (CurrentViewTarget == this);

    //Nothing to do if state has not changed
    if (bShouldBeActive == bIsActiveViewTarget)
        return;

    bIsActiveViewTarget = bShouldBeActive;

    if (!bIsActiveViewTarget)
    {
        UE_LOG(LogTemp, Log, TEXT("CameraRail [%s]: Auto-deactivating (no longer view target)"), *GetName());
        StopAllShakes(true);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("CameraRail [%s]: Auto-activating (became view target)"), *GetName());
    }
}

void ACameraRail::HandlePlayerFollowing(float DeltaTime)
{
    // ==================================================================================
    // HandlePlayerFollowing
    // Update TargetPosition along the spline when the player has moved beyond a small
    // threshold. Initializes from the global closest point once, then uses a local
    // spline search to prevent switching to another nearby curve section. Applies
    // the spline-distance constraint if enabled.
    // ==================================================================================
    if (!PlayerReference || !SplineComponent)
        return;

    if (!IsValid(PlayerReference))
    {
        PlayerReference = nullptr;
        return;
    }

    FVector PlayerLocation = PlayerReference->GetActorLocation();

    if (!bPlayerLocationInitialized)
    {
        LastPlayerLocation = PlayerLocation;
        bPlayerLocationInitialized = true;
    }

    float DistanceSquared = FVector::DistSquared(PlayerLocation, LastPlayerLocation);
    if (DistanceSquared < PlayerMovementThresholdSquared)
    {
        return;
    }

    LastPlayerLocation = PlayerLocation;

    float ClosestDistance;
    if (bHasFollowTargetPosition)
    {
        const float ClosestPosition = FindClosestPositionToLocationNear(PlayerLocation, TargetPosition);
        ClosestDistance = ClosestPosition * GetRailLength();
    }
    else
    {
        const float ClosestInputKey = SplineComponent->FindInputKeyClosestToWorldLocation(PlayerLocation);
        ClosestDistance = SplineComponent->GetDistanceAlongSplineAtSplineInputKey(ClosestInputKey);
        bHasFollowTargetPosition = true;
    }

    float RailLength = GetRailLength();
    if (RailLength > 0.0f)
    {
        float NewTargetPosition = ClosestDistance / RailLength;
        NewTargetPosition = FMath::Clamp(NewTargetPosition, 0.0f, 1.0f);

        if (bEnforceMinimumDistanceOnSpline)
        {
            ApplySplineDistanceConstraint(NewTargetPosition, PlayerLocation);
        }

        TargetPosition = NewTargetPosition;
    }
}

void ACameraRail::ApplySplineDistanceConstraint(float& InOutTargetPosition, const FVector& PlayerLocation)
{
    // ==================================================================================
    // ApplySplineDistanceConstraint
    // Ensures the camera's target position on the spline stays at least
    // MinimumDistanceToPlayer away from the player by nudging the target toward a
    // position that satisfies the minimum distance, blending the correction by
    // SplineDistanceCorrectionSpeed. The correction search remains local to the
    // current rail position so it cannot jump to another nearby spline section.
    // ==================================================================================
    if (!SplineComponent)
        return;

    float RailLength = GetRailLength();
    if (RailLength <= 0.0f)
        return;

    FVector SplinePointAtTarget = GetLocationAtPosition(InOutTargetPosition);
    float CurrentDistance = FVector::Dist(SplinePointAtTarget, PlayerLocation);

    if (CurrentDistance < MinimumDistanceToPlayer)
    {
        float CorrectionAlpha = FMath::Clamp(SplineDistanceCorrectionSpeed * GetWorld()->GetDeltaSeconds(), 0.0f, 1.0f);

        FVector DirectionToPlayer = (PlayerLocation - SplinePointAtTarget).GetSafeNormal();
        FVector DesiredCameraLocation = PlayerLocation - (DirectionToPlayer * MinimumDistanceToPlayer);

        float ClosestToDesired = FindClosestPositionToLocationNear(DesiredCameraLocation, InOutTargetPosition);

        InOutTargetPosition = FMath::Lerp(InOutTargetPosition, ClosestToDesired, CorrectionAlpha);
    }
}

void ACameraRail::ApplyOffsetDistanceConstraint(FVector& InOutCameraLocation, const FVector& PlayerLocation, const FRotator& SplineRotation)
{
    // ==================================================================================
    // ApplyOffsetDistanceConstraint
    // Enforces XY-only min/max distances between camera (with offset) and player.
    // Computes required local offset adjustments (respecting bUseOffsetX/Y) and
    // applies them while preserving Z (altitude).
    // ==================================================================================
    FVector SplinePoint = InOutCameraLocation - SplineRotation.RotateVector(CameraOffset);

    FVector SplineToPlayer = PlayerLocation - SplinePoint;
    FVector SplineToPlayerXY = SplineToPlayer;
    SplineToPlayerXY.Z = 0.0f;

    float DistanceXY = SplineToPlayerXY.Size();

    if (DistanceXY < MinimumOffsetDistance || DistanceXY > MaximumOffsetDistance)
    {
        float TargetDistance = FMath::Clamp(DistanceXY, MinimumOffsetDistance, MaximumOffsetDistance);
        FVector DirectionXY = SplineToPlayerXY.GetSafeNormal();
        FVector TargetCameraLocationXY = SplinePoint + (DirectionXY * TargetDistance);
        TargetCameraLocationXY.Z = SplinePoint.Z;

        FVector RequiredWorldOffset = TargetCameraLocationXY - SplinePoint;
        FVector LocalOffset = SplineRotation.UnrotateVector(RequiredWorldOffset);

        if (!bUseOffsetX)
        {
            LocalOffset.X = CameraOffset.X;
        }
        if (!bUseOffsetY)
        {
            LocalOffset.Y = CameraOffset.Y;
        }

        LocalOffset.Z = CameraOffset.Z;

        FVector FinalWorldOffset = SplineRotation.RotateVector(LocalOffset);
        InOutCameraLocation = SplinePoint + FinalWorldOffset;
    }
}

void ACameraRail::DrawDebugInfo()
{
    // ==================================================================================
    // DrawDebugInfo
    // Visual debugging helpers: draws camera position, forward vector, player link
    // and distance circles used by the offset/spline minimum distance systems.
    // ==================================================================================
    UWorld* World = GetWorld();
    if (!World || !SplineComponent)
        return;

    FVector CurrentLocation = CameraComponent->GetComponentLocation();
    DrawDebugSphere(World, CurrentLocation, DebugCameraSphereRadius, 12, FColor::Red, false, 0.0f, 0, 3.0f);

    FVector ForwardVector = CameraComponent->GetForwardVector();
    FVector EndPoint = CurrentLocation + (ForwardVector * DebugDirectionArrowLength);
    DrawDebugDirectionalArrow(World, CurrentLocation, EndPoint, 20.0f, FColor::Blue, false, 0.0f, 0, 3.0f);

    if (PlayerReference && IsValid(PlayerReference))
    {
        FVector PlayerLocation = PlayerReference->GetActorLocation();
        DrawDebugLine(World, CurrentLocation, PlayerLocation, FColor::Green, false, 0.0f, 0, 2.0f);
        DrawDebugSphere(World, PlayerLocation, DebugPlayerSphereRadius, 8, FColor::Green, false, 0.0f, 0, 2.0f);

        if (bEnforceMinimumDistanceOnSpline)
        {
            float Distance = FVector::Dist(CurrentLocation, PlayerLocation);
            FColor DistanceColor = Distance < MinimumDistanceToPlayer ? FColor::Orange : FColor::Cyan;

            DrawDebugSphere(World, PlayerLocation, MinimumDistanceToPlayer, 32, DistanceColor, false, 0.0f, 0, 1.0f);
        }

        if (bEnforceMinimumDistanceWithOffset)
        {
            FVector CameraToPlayer = PlayerLocation - CurrentLocation;
            FVector CameraToPlayerXY = CameraToPlayer;
            CameraToPlayerXY.Z = 0.0f;
            float DistanceXY = CameraToPlayerXY.Size();

            FColor MinColor = DistanceXY < MinimumOffsetDistance ? FColor::Red : FColor::Yellow;
            FColor MaxColor = DistanceXY > MaximumOffsetDistance ? FColor::Purple : FColor::Cyan;

            FVector PlayerLocationXY = PlayerLocation;
            PlayerLocationXY.Z = CurrentLocation.Z;

            DrawDebugCircle(World, PlayerLocationXY, MinimumOffsetDistance, 32, MinColor, false, 0.0f, 0, 2.0f, FVector(0, 1, 0), FVector(0, 0, 1), false);
            DrawDebugCircle(World, PlayerLocationXY, MaximumOffsetDistance, 32, MaxColor, false, 0.0f, 0, 2.0f, FVector(0, 1, 0), FVector(0, 0, 1), false);

            DrawDebugLine(World, CurrentLocation, PlayerLocationXY, FColor::Magenta, false, 0.0f, 0, 2.0f);
        }
    }
}

float ACameraRail::GetSpeedMultiplierAtPosition(float Position) const
{
    // ==================================================================================
    // GetSpeedMultiplierAtPosition
    // Returns a multiplier from the optional SpeedCurve; defaults to 1.0 if none.
    // ==================================================================================
    if (SpeedCurve)
    {
        return SpeedCurve->GetFloatValue(Position);
    }
    return 1.0f;
}

void ACameraRail::FindAndSetPlayer()
{
    // ==================================================================================
    // FindAndSetPlayer
    // Periodic timer callback that attempts to locate the player pawn from the
    // world's first player controller. Stops the search after MaxPlayerSearchAttempts
    // and logs a warning so the user can manually set the reference if needed.
    // ==================================================================================
    if (PlayerReference && IsValid(PlayerReference))
    {
        UWorld* World = GetWorld();
        if (World)
        {
            World->GetTimerManager().ClearTimer(PlayerSearchTimer);
        }
        return;
    }

    PlayerSearchAttempts++;

    if (PlayerSearchAttempts >= MaxPlayerSearchAttempts)
    {
        UE_LOG(LogTemp, Warning, TEXT("CameraRail: Player search timeout after %d attempts. Call SetPlayerReference manually."), MaxPlayerSearchAttempts);

        UWorld* World = GetWorld();
        if (World)
        {
            World->GetTimerManager().ClearTimer(PlayerSearchTimer);
        }
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
        return;

    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (APawn* PlayerPawn = PC->GetPawn())
        {
            if (IsValid(PlayerPawn))
            {
                SetPlayerReference(PlayerPawn);

                if (bAutoActivateOnPlay)
                {
                    ActivateCamera();
                    UE_LOG(LogTemp, Warning, TEXT("CameraRail: Auto-activated camera on %s"), *GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Log, TEXT("CameraRail: Found player but auto-activation is disabled on %s"), *GetName());
                }

                World->GetTimerManager().ClearTimer(PlayerSearchTimer);

                UE_LOG(LogTemp, Warning, TEXT("CameraRail: Found and set player reference to %s after %d attempts"), *PlayerPawn->GetName(), PlayerSearchAttempts);
                return;
            }
        }
    }

    if (PlayerSearchAttempts % PlayerSearchLogInterval == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("CameraRail: Still searching for player... (attempt %d/%d)"), PlayerSearchAttempts, MaxPlayerSearchAttempts);
    }
}

// ==================================================================================
// CINEMATIC CAMERA FOCUS SETTINGS
// ==================================================================================

void ACameraRail::UpdateCameraFocusSettings()
{
    if (!CameraComponent)
        return;

    if (bEnableDepthOfField)
    {
        CameraComponent->CurrentAperture = DepthOfFieldAperture;
    }

    if (bEnableFocusTracking && PlayerReference && IsValid(PlayerReference))
    {
        CameraComponent->FocusSettings.FocusMethod = ECameraFocusMethod::Tracking;
        CameraComponent->FocusSettings.TrackingFocusSettings.ActorToTrack = PlayerReference;
        CameraComponent->FocusSettings.TrackingFocusSettings.RelativeOffset = FocusTrackingOffset;

        UE_LOG(LogTemp, Log, TEXT("CameraRail [%s]: Focus tracking enabled for player"), *GetName());
    }
    else if (!bEnableFocusTracking)
    {
        CameraComponent->FocusSettings.FocusMethod = ECameraFocusMethod::DoNotOverride;
    }
}

void ACameraRail::SetDepthOfFieldEnabled(bool bEnabled)
{
    bEnableDepthOfField = bEnabled;
    UpdateCameraFocusSettings();
}

void ACameraRail::SetFocusTrackingEnabled(bool bEnabled)
{
    bEnableFocusTracking = bEnabled;
    UpdateCameraFocusSettings();
}

// ==================================================================================
// CAMERA SHAKE SYSTEM
// ==================================================================================

void ACameraRail::UpdateCameraShake(float DeltaTime)
{
    // ==================================================================================
    // UpdateCameraShake
    // Manages movement/idle camera shakes based on player speed and config flags.
    // Starts/stops appropriate shake instances on the PlayerCameraManager and
    // scales movement shake by speed when configured.
    // ==================================================================================
    if (!bEnableCameraShake)
    {
        StopAllShakes(true);
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
        return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC || !PC->PlayerCameraManager)
        return;

    float PlayerSpeed = 0.0f;
    bool bPlayerIsMoving = false;

    if (bUsePlayerSpeed && PlayerReference && IsValid(PlayerReference))
    {
        PlayerSpeed = CalculatePlayerSpeed(DeltaTime);
        ShakeState.CurrentPlayerSpeed = PlayerSpeed;
        bPlayerIsMoving = PlayerSpeed >= MinSpeedForShake;
        ShakeState.bIsPlayerMoving = bPlayerIsMoving;
    }

    bool bShouldPlayMovementShake = false;
    float MovementShakeScale = ShakeScale;

    if (bUsePlayerSpeed)
    {
        if (bPlayerIsMoving)
        {
            bShouldPlayMovementShake = true;

            if (bScaleShakeWithSpeed)
            {
                float SpeedRatio = FMath::Clamp(PlayerSpeed / MaxSpeedForMaxShake, 0.0f, 1.0f);
                MovementShakeScale = ShakeScale * SpeedRatio;
            }
        }
    }
    else
    {
        bShouldPlayMovementShake = CameraShakeClass != nullptr;
    }

    if (bShouldPlayMovementShake && CameraShakeClass)
    {
        StartMovementShake(MovementShakeScale);
        StopIdleShake(true);
    }
    else
    {
        StopMovementShake(true);

        if (bEnableIdleShake && IdleShakeClass)
        {
            StartIdleShake();
        }
        else
        {
            StopIdleShake(true);
        }
    }
}

float ACameraRail::CalculatePlayerSpeed(float DeltaTime)
{
    // ==================================================================================
    // CalculatePlayerSpeed
    // Returns player's instantaneous speed computed from last stored location and
    // updates ShakeState.LastPlayerLocation for the next sample.
    // ==================================================================================
    if (!PlayerReference || !IsValid(PlayerReference) || DeltaTime <= 0.0f)
        return 0.0f;

    FVector CurrentPlayerLocation = PlayerReference->GetActorLocation();
    float Speed = FVector::Dist(CurrentPlayerLocation, ShakeState.LastPlayerLocation) / DeltaTime;
    ShakeState.LastPlayerLocation = CurrentPlayerLocation;

    return Speed;
}

void ACameraRail::StartMovementShake(float Intensity)
{
    // ==================================================================================
    // StartMovementShake
    // Requests a camera shake instance from the PlayerCameraManager for movement
    // if one is not already active. Intensity scales the shake.
    // ==================================================================================
    if (!CameraShakeClass)
        return;

    UWorld* World = GetWorld();
    if (!World)
        return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC || !PC->PlayerCameraManager)
        return;

    APlayerCameraManager* CameraManager = PC->PlayerCameraManager;

    if (!ShakeState.ActiveMovementShake)
    {
        ShakeState.ActiveMovementShake = CameraManager->StartCameraShake(CameraShakeClass, Intensity);
    }
}

void ACameraRail::StartIdleShake()
{
    // ==================================================================================
    // StartIdleShake
    // Requests the idle shake instance from the PlayerCameraManager when enabled
    // and no movement shake is playing.
    // ==================================================================================
    if (!IdleShakeClass)
        return;

    UWorld* World = GetWorld();
    if (!World)
        return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC || !PC->PlayerCameraManager)
        return;

    APlayerCameraManager* CameraManager = PC->PlayerCameraManager;

    if (!ShakeState.ActiveIdleShake)
    {
        ShakeState.ActiveIdleShake = CameraManager->StartCameraShake(IdleShakeClass, IdleShakeScale);
    }
}

void ACameraRail::StopMovementShake(bool bImmediate)
{
    // ==================================================================================
    // StopMovementShake
    // Stops the active movement shake instance (if any) via PlayerCameraManager.
    // ==================================================================================
    if (!ShakeState.ActiveMovementShake)
        return;

    UWorld* World = GetWorld();
    if (!World)
        return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC || !PC->PlayerCameraManager)
        return;

    APlayerCameraManager* CameraManager = PC->PlayerCameraManager;
    CameraManager->StopCameraShake(ShakeState.ActiveMovementShake, bImmediate);
    ShakeState.ActiveMovementShake = nullptr;
}

void ACameraRail::StopIdleShake(bool bImmediate)
{
    // ==================================================================================
    // StopIdleShake
    // Stops the active idle shake instance (if any) via PlayerCameraManager.
    // ==================================================================================
    if (!ShakeState.ActiveIdleShake)
        return;

    UWorld* World = GetWorld();
    if (!World)
        return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC || !PC->PlayerCameraManager)
        return;

    APlayerCameraManager* CameraManager = PC->PlayerCameraManager;
    CameraManager->StopCameraShake(ShakeState.ActiveIdleShake, bImmediate);
    ShakeState.ActiveIdleShake = nullptr;
}

void ACameraRail::StopAllShakes(bool bImmediate)
{
    // ==================================================================================
    // StopAllShakes
    // Convenience wrapper that stops both movement and idle shakes.
    // ==================================================================================
    StopMovementShake(bImmediate);
    StopIdleShake(bImmediate);
}

// ==================================================================================
// PUBLIC INTERFACE
// ==================================================================================

void ACameraRail::SetRailPosition(float Position, bool bTeleport)
{
    // ==================================================================================
    // SetRailPosition
    // Public API to move the rail's target (and optionally current) normalized
    // position. Teleport applies the position immediately without smoothing.
    // ==================================================================================
    TargetPosition = FMath::Clamp(Position, 0.0f, 1.0f);

    if (bTeleport)
    {
        CurrentPosition = TargetPosition;
        UpdateCameraPosition(0.0f);
    }
}

void ACameraRail::SetPlayerReference(AActor* NewPlayer)
{
    // ==================================================================================
    // SetPlayerReference
    // Assigns the actor to follow/track. Initializes internal tracking state and
    // optionally primes camera shake state. Logs on invalid assignment.
    // ==================================================================================
    if (NewPlayer && IsValid(NewPlayer))
    {
        PlayerReference = NewPlayer;

        LastPlayerLocation = NewPlayer->GetActorLocation();
        bPlayerLocationInitialized = true;

        if (bEnableCameraShake)
        {
            ShakeState.LastPlayerLocation = NewPlayer->GetActorLocation();
        }

        UpdateCameraFocusSettings();

        UE_LOG(LogTemp, Log, TEXT("CameraRail: Player reference set to %s"), *NewPlayer->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CameraRail: Attempted to set invalid player reference"));
        PlayerReference = nullptr;
        bPlayerLocationInitialized = false;
    }
}

float ACameraRail::GetRailLength() const
{
    // ==================================================================================
    // GetRailLength
    // Utility that returns the spline length or 0 if invalid.
    // ==================================================================================
    return SplineComponent ? SplineComponent->GetSplineLength() : 0.0f;
}

FVector ACameraRail::GetLocationAtPosition(float Position) const
{
    // ==================================================================================
    // GetLocationAtPosition
    // Converts a normalized position [0,1] to world location along the spline.
    // ==================================================================================
    if (!SplineComponent)
        return FVector::ZeroVector;

    float Distance = FMath::Clamp(Position, 0.0f, 1.0f) * GetRailLength();
    return SplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
}

FRotator ACameraRail::GetRotationAtPosition(float Position) const
{
    // ==================================================================================
    // GetRotationAtPosition
    // Converts a normalized position [0,1] to world rotation along the spline.
    // ==================================================================================
    if (!SplineComponent)
        return FRotator::ZeroRotator;

    float Distance = FMath::Clamp(Position, 0.0f, 1.0f) * GetRailLength();
    return SplineComponent->GetRotationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
}

float ACameraRail::FindClosestPositionToLocation(const FVector& WorldLocation) const
{
    // ==================================================================================
    // FindClosestPositionToLocation
    // Finds the normalized spline position closest to a world location. Uses spline
    // input keys normalized by the number of spline segments.
    // ==================================================================================
    if (!SplineComponent)
        return 0.0f;

    float ClosestInputKey = SplineComponent->FindInputKeyClosestToWorldLocation(WorldLocation);
    float NumberOfPoints = SplineComponent->GetNumberOfSplinePoints() - 1;

    return NumberOfPoints > 0 ? ClosestInputKey / NumberOfPoints : 0.0f;
}

float ACameraRail::FindClosestPositionToLocationNear(const FVector& WorldLocation, float CenterPosition) const
{
    // ==================================================================================
    // FindClosestPositionToLocationNear
    // Searches a limited normalized range around the current rail position instead of
    // querying the entire spline. This prevents the camera from switching to another
    // nearby section when a curved spline passes close to itself.
    // ==================================================================================
    if (!SplineComponent || LocalClosestSearchSamples <= 0)
        return FMath::Clamp(CenterPosition, 0.0f, 1.0f);

    const bool bIsClosedLoop = SplineComponent->IsClosedLoop();
    float BestPosition = FMath::Clamp(CenterPosition, 0.0f, 1.0f);
    float BestDistanceSquared = TNumericLimits<float>::Max();

    for (int32 SampleIndex = 0; SampleIndex <= LocalClosestSearchSamples; ++SampleIndex)
    {
        const float Alpha = static_cast<float>(SampleIndex) / LocalClosestSearchSamples;
        float CandidatePosition = CenterPosition +
            FMath::Lerp(-LocalClosestSearchRange, LocalClosestSearchRange, Alpha);

        if (bIsClosedLoop)
        {
            CandidatePosition = FMath::Fmod(CandidatePosition, 1.0f);
            if (CandidatePosition < 0.0f)
            {
                CandidatePosition += 1.0f;
            }
        }
        else
        {
            CandidatePosition = FMath::Clamp(CandidatePosition, 0.0f, 1.0f);
        }

        const FVector CandidateLocation = GetLocationAtPosition(CandidatePosition);
        const float DistanceSquared = FVector::DistSquared(CandidateLocation, WorldLocation);
        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestPosition = CandidatePosition;
        }
    }

    return BestPosition;
}

void ACameraRail::ActivateCamera()
{
    // ==================================================================================
    // ActivateCamera
    // Sets this actor as the PlayerController's view target, activating rail updates
    // and camera behavior. Validates core components and logs failures.
    // ==================================================================================
    if (!CameraComponent || !IsValid(CameraComponent))
    {
        UE_LOG(LogTemp, Error, TEXT("CameraRail [%s]: Cannot activate - CameraComponent invalid"), *GetName());
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("CameraRail [%s]: Cannot activate - World invalid"), *GetName());
        return;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("CameraRail [%s]: No PlayerController found"), *GetName());
        return;
    }

    PC->SetViewTarget(this);

    UE_LOG(LogTemp, Log, TEXT("CameraRail [%s]: Camera activated"), *GetName());
}

void ACameraRail::DeactivateCamera()
{
    // ==================================================================================
    // DeactivateCamera
    // Restores the player's pawn as view target if available.
    // ==================================================================================
    UWorld* World = GetWorld();
    if (!World)
        return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
        return;

    if (PlayerReference && IsValid(PlayerReference))
    {
        PC->SetViewTarget(PlayerReference);
        UE_LOG(LogTemp, Log, TEXT("CameraRail [%s]: Deactivated - view target set to player"), *GetName());
    }
}