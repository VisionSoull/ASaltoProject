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

void ACameraRail::BecomeViewTarget(APlayerController* PC)
{
    Super::BecomeViewTarget(PC);

    bIsActiveViewTarget = true;

    UE_LOG(LogTemp, Log, TEXT("CameraRail [%s]: Became active view target"), *GetName());
}

void ACameraRail::EndViewTarget(APlayerController* PC)
{
    Super::EndViewTarget(PC);

    bIsActiveViewTarget = false;

    UE_LOG(LogTemp, Log, TEXT("CameraRail [%s]: No longer view target"), *GetName());
}

void ACameraRail::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
    Super::OnConstruction(Transform);

    if (SplineComponent)
    {
        SplineComponent->SetDrawDebug(bDrawSpline);
    }

    UpdateCameraFocusSettings();
}
#endif

void ACameraRail::UpdateCameraPosition(float DeltaTime)
{
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
    return ShakeState.CurrentPlayerSpeed < PlayerStoppedThreshold;
}

void ACameraRail::CheckViewTargetStatus()
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
        return;

    AActor* CurrentViewTarget = PC->GetViewTarget();
    bool bShouldBeActive = (CurrentViewTarget == this);

    if (bShouldBeActive != bIsActiveViewTarget)
    {
        if (!bShouldBeActive && bIsActiveViewTarget)
        {
            UE_LOG(LogTemp, Log, TEXT("CameraRail [%s]: Auto-deactivating (no longer view target)"), *GetName());
            bIsActiveViewTarget = false;
            StopAllShakes(true);
        }
        else if (bShouldBeActive && !bIsActiveViewTarget)
        {
            UE_LOG(LogTemp, Log, TEXT("CameraRail [%s]: Auto-activating (became view target)"), *GetName());
            bIsActiveViewTarget = true;
        }
    }
}

void ACameraRail::HandlePlayerFollowing(float DeltaTime)
{
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

    float ClosestInputKey = SplineComponent->FindInputKeyClosestToWorldLocation(PlayerLocation);
    float ClosestDistance = SplineComponent->GetDistanceAlongSplineAtSplineInputKey(ClosestInputKey);

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

        float ClosestToDesired = FindClosestPositionToLocation(DesiredCameraLocation);

        InOutTargetPosition = FMath::Lerp(InOutTargetPosition, ClosestToDesired, CorrectionAlpha);
    }
}

void ACameraRail::ApplyOffsetDistanceConstraint(FVector& InOutCameraLocation, const FVector& PlayerLocation, const FRotator& SplineRotation)
{
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
    if (SpeedCurve)
    {
        return SpeedCurve->GetFloatValue(Position);
    }
    return 1.0f;
}

void ACameraRail::FindAndSetPlayer()
{
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
    if (!PlayerReference || !IsValid(PlayerReference) || DeltaTime <= 0.0f)
        return 0.0f;

    FVector CurrentPlayerLocation = PlayerReference->GetActorLocation();
    float Speed = FVector::Dist(CurrentPlayerLocation, ShakeState.LastPlayerLocation) / DeltaTime;
    ShakeState.LastPlayerLocation = CurrentPlayerLocation;

    return Speed;
}

void ACameraRail::StartMovementShake(float Intensity)
{
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
    StopMovementShake(bImmediate);
    StopIdleShake(bImmediate);
}

// ==================================================================================
// PUBLIC INTERFACE
// ==================================================================================

void ACameraRail::SetRailPosition(float Position, bool bTeleport)
{
    TargetPosition = FMath::Clamp(Position, 0.0f, 1.0f);

    if (bTeleport)
    {
        CurrentPosition = TargetPosition;
        UpdateCameraPosition(0.0f);
    }
}

void ACameraRail::SetPlayerReference(AActor* NewPlayer)
{
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
    return SplineComponent ? SplineComponent->GetSplineLength() : 0.0f;
}

FVector ACameraRail::GetLocationAtPosition(float Position) const
{
    if (!SplineComponent)
        return FVector::ZeroVector;

    float Distance = FMath::Clamp(Position, 0.0f, 1.0f) * GetRailLength();
    return SplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
}

FRotator ACameraRail::GetRotationAtPosition(float Position) const
{
    if (!SplineComponent)
        return FRotator::ZeroRotator;

    float Distance = FMath::Clamp(Position, 0.0f, 1.0f) * GetRailLength();
    return SplineComponent->GetRotationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
}

float ACameraRail::FindClosestPositionToLocation(const FVector& WorldLocation) const
{
    if (!SplineComponent)
        return 0.0f;

    float ClosestInputKey = SplineComponent->FindInputKeyClosestToWorldLocation(WorldLocation);
    float NumberOfPoints = SplineComponent->GetNumberOfSplinePoints() - 1;

    return NumberOfPoints > 0 ? ClosestInputKey / NumberOfPoints : 0.0f;
}

void ACameraRail::ActivateCamera()
{
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