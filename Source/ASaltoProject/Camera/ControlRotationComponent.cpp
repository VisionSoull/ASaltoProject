#include "ControlRotationComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"

UControlRotationComponent::UControlRotationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UControlRotationComponent::BeginPlay()
{
    Super::BeginPlay();
    // ==================================================================================
    // BeginPlay
    // - Caches the owning PlayerController and initial view target
    // - Initializes movement and camera rotation tracking state
    // - Optionally outputs debug information
    // ==================================================================================

    OwnerPC = Cast<APlayerController>(GetOwner());

    if (!OwnerPC)
    {
        UE_LOG(LogTemp, Error, TEXT("ControlRotationComponent: Owner is not a PlayerController! Component will not function."));
        SetComponentTickEnabled(false);
        return;
    }

    LastViewTarget = OwnerPC->GetViewTarget();

    if (APawn* ControlledPawn = OwnerPC->GetPawn())
    {
        LastMovementInputVector = ControlledPawn->GetLastMovementInputVector();
        bHasLastMovementInput = true;
    }

    if (UCameraComponent* CameraComp = GetViewTargetCameraComponent())
    {
        LastCameraYaw = CameraComp->GetForwardVector().Rotation().Yaw;
        bHasLastCameraYaw = true;
    }

    if (bShowDebugInfo)
    {
        UE_LOG(LogTemp, Warning, TEXT("========================================"));
        UE_LOG(LogTemp, Warning, TEXT("ControlRotationComponent: BEGIN PLAY"));
        UE_LOG(LogTemp, Warning, TEXT("  Owner: %s"), *OwnerPC->GetName());
        UE_LOG(LogTemp, Warning, TEXT("  bEnableControlRotationAlignment: %s"), bEnableControlRotationAlignment ? TEXT("TRUE") : TEXT("FALSE"));
        UE_LOG(LogTemp, Warning, TEXT("  Initial View Target: %s"), LastViewTarget ? *LastViewTarget->GetName() : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("  Blend-aware synchronization enabled"));
        UE_LOG(LogTemp, Warning, TEXT("  Updates every OTHER frame for performance"));
        UE_LOG(LogTemp, Warning, TEXT("========================================"));
    }
}

void UControlRotationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    // ==================================================================================
    // TickComponent
    // - Detects view target and camera rotation changes
    // - Tracks changes to the player's movement direction
    // - Preserves movement direction while the camera rotates
    // - Updates control rotation every other frame for performance
    // ==================================================================================

    if (!bEnableControlRotationAlignment || !OwnerPC)
        return;

    // Performance optimization: only update every other frame
    bFrameToggle = !bFrameToggle;
    if (!bFrameToggle)
        return;

    // Check for view target changes
    AActor* CurrentViewTarget = OwnerPC->GetViewTarget();
    if (CurrentViewTarget != LastViewTarget)
    {
        OnViewTargetChanged();
        LastViewTarget = CurrentViewTarget;
    }

    const bool bMovementDirectionChanged = HasMovementDirectionChanged();
    if (bMovementDirectionChanged)
    {
        bPreserveMovementDirection = false;
        bIsBlending = false;
    }

    if (UCameraComponent* CameraComp = GetViewTargetCameraComponent())
    {
        // Detect camera yaw changes independently of view target changes
        const float CurrentCameraYaw = CameraComp->GetForwardVector().Rotation().Yaw;
        if (bHasLastCameraYaw &&
            FMath::Abs(FMath::FindDeltaAngleDegrees(LastCameraYaw, CurrentCameraYaw)) > 0.1f &&
            !bMovementDirectionChanged &&
            LastMovementInputVector.SizeSquared2D() > KINDA_SMALL_NUMBER)
        {
            bPreserveMovementDirection = true;
            bIsBlending = false;
        }

        LastCameraYaw = CurrentCameraYaw;
        bHasLastCameraYaw = true;
    }

    // Update control rotation alignment
    UpdateControlRotation(DeltaTime);
}

bool UControlRotationComponent::HasMovementDirectionChanged()
{
    // ==================================================================================
    // HasMovementDirectionChanged
    // - Reads the last movement input consumed by the controlled pawn
    // - Detects changes in direction or input magnitude
    // - Stores the current vector for comparison on the next update
    // ==================================================================================

    if (!OwnerPC)
        return false;

    APawn* ControlledPawn = OwnerPC->GetPawn();
    if (!ControlledPawn)
        return false;

    const FVector CurrentMovementInputVector = ControlledPawn->GetLastMovementInputVector();
    const bool bChanged = !bHasLastMovementInput ||
        !CurrentMovementInputVector.Equals(LastMovementInputVector, 0.01f);

    LastMovementInputVector = CurrentMovementInputVector;
    bHasLastMovementInput = true;
    return bChanged;
}

void UControlRotationComponent::AlignControlRotationToCamera()
{
    // ==================================================================================
    // AlignControlRotationToCamera
    // - Gets the active camera forward direction
    // - Removes pitch and roll so only the horizontal direction is used
    // - Applies the resulting yaw to the PlayerController
    // ==================================================================================

    UCameraComponent* CameraComp = GetViewTargetCameraComponent();
    if (!OwnerPC || !CameraComp)
        return;

    FRotator CameraForward = CameraComp->GetForwardVector().Rotation();
    CameraForward.Pitch = 0.0f;
    CameraForward.Roll = 0.0f;
    OwnerPC->SetControlRotation(CameraForward);
}

void UControlRotationComponent::OnViewTargetChanged()
{
    // ==================================================================================
    // OnViewTargetChanged
    // - Logs the previous and new view targets when debugging is enabled
    // - Preserves the current movement direction while the player is moving
    // - Aligns immediately when the player is not moving and no blend is active
    // ==================================================================================

    if (!OwnerPC)
        return;

    AActor* NewViewTarget = OwnerPC->GetViewTarget();

    if (bShowDebugInfo)
    {
        UE_LOG(LogTemp, Warning, TEXT("========================================"));
        UE_LOG(LogTemp, Warning, TEXT("ControlRotationComponent: VIEW TARGET CHANGED"));
        UE_LOG(LogTemp, Warning, TEXT("  Old View Target: %s"), LastViewTarget ? *LastViewTarget->GetName() : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("  New View Target: %s"), NewViewTarget ? *NewViewTarget->GetName() : TEXT("NULL"));
    }

    const bool bHasMovementInput = LastMovementInputVector.SizeSquared2D() > KINDA_SMALL_NUMBER;

    // Keep the current control rotation while the player is moving
    // This prevents a camera change from changing the world direction of held movement input
    if (bHasMovementInput)
    {
        bPreserveMovementDirection = true;

        if (bShowDebugInfo)
        {
            UE_LOG(LogTemp, Log, TEXT("  Movement active - preserving current control direction"));
        }
    }
    else if (!bIsBlending)
    {
        AlignControlRotationToCamera();
    }
    else
    {
        if (bShowDebugInfo)
        {
            UE_LOG(LogTemp, Log, TEXT("  Blend active - continuing synchronization"));
        }
    }

    if (bShowDebugInfo)
    {
        UE_LOG(LogTemp, Warning, TEXT("========================================"));
    }
}

void UControlRotationComponent::NotifyCameraBlend(float InBlendDuration)
{
    // ==================================================================================
    // NotifyCameraBlend
    // - Receives camera blend notifications from Blueprint
    // - Performs an immediate alignment for zero-length transitions
    // - Starts timed synchronization for transitions with a duration
    // - Preserves active movement direction during camera transitions
    // ==================================================================================

    if (!OwnerPC || !bEnableControlRotationAlignment)
        return;

    if (InBlendDuration <= 0.0f)
    {
        if (LastMovementInputVector.SizeSquared2D() > KINDA_SMALL_NUMBER)
        {
            bPreserveMovementDirection = true;
            bIsBlending = false;
            return;
        }

        // Instant transition
        AlignControlRotationToCamera();
        return;
    }

    // Start blend synchronization
    StartBlendSync(InBlendDuration);

    if (bShowDebugInfo)
    {
        UE_LOG(LogTemp, Warning, TEXT("ControlRotationComponent: Camera blend notified - Duration=%.2fs"), InBlendDuration);
    }
}

float UControlRotationComponent::EstimateBlendDuration() const
{
    // ==================================================================================
    // EstimateBlendDuration
    // Legacy placeholder kept for compatibility. Blueprints now notify blends
    // directly, so this returns a default value.
    // ==================================================================================

    return 1.0f;
}

void UControlRotationComponent::StartBlendSync(float Duration)
{
    // ==================================================================================
    // StartBlendSync
    // - Initializes blend timing and starting rotation state
    // - Calculates the target yaw from the active camera
    // - Skips synchronization when active movement direction must be preserved
    // ==================================================================================

    if (!OwnerPC)
        return;

    UCameraComponent* TargetCamera = GetViewTargetCameraComponent();
    if (!TargetCamera)
        return;

    bIsBlending = true;
    BlendDuration = Duration;
    BlendStartTime = GetWorld()->GetTimeSeconds();
    BlendStartRotation = OwnerPC->GetControlRotation();

    if (LastMovementInputVector.SizeSquared2D() > KINDA_SMALL_NUMBER)
    {
        bPreserveMovementDirection = true;
        bIsBlending = false;
        return;
    }

    // Target rotation from the new camera's forward vector (yaw only)
    BlendTargetRotation = TargetCamera->GetForwardVector().Rotation();
    BlendTargetRotation.Pitch = 0.0f;
    BlendTargetRotation.Roll = 0.0f;

    if (bShowDebugInfo)
    {
        UE_LOG(LogTemp, Log, TEXT("ControlRotationComponent: Starting blend sync"));
        UE_LOG(LogTemp, Log, TEXT("  Duration: %.2fs"), BlendDuration);
        UE_LOG(LogTemp, Log, TEXT("  Start Rotation: %s"), *BlendStartRotation.ToString());
        UE_LOG(LogTemp, Log, TEXT("  Target Rotation: %s"), *BlendTargetRotation.ToString());
        UE_LOG(LogTemp, Log, TEXT("  Yaw Delta: %.2f degrees"),
            FMath::Abs(FMath::FindDeltaAngleDegrees(BlendStartRotation.Yaw, BlendTargetRotation.Yaw)));
    }
}

void UControlRotationComponent::UpdateControlRotation(float DeltaTime)
{
    // ==================================================================================
    // UpdateControlRotation
    // - Preserves control rotation while movement direction is locked
    // - Interpolates control rotation during an active camera blend
    // - Aligns control rotation with the camera when no blend is active
    // ==================================================================================

    if (!OwnerPC)
        return;

    UCameraComponent* CameraComp = GetViewTargetCameraComponent();
    if (!CameraComp)
        return;

    if (bPreserveMovementDirection)
    {
        bIsBlending = false;
        return;
    }

    if (bIsBlending)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
        float ElapsedTime = CurrentTime - BlendStartTime;

        if (ElapsedTime < BlendDuration)
        {
            // Calculate blend alpha matching the camera blend
            float Alpha = FMath::Clamp(ElapsedTime / BlendDuration, 0.0f, 1.0f);

            // Use the same easing as SetViewTargetWithBlend (cubic ease in/out)
            Alpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);

            FRotator NewRotation = FMath::Lerp(BlendStartRotation, BlendTargetRotation, Alpha);
            OwnerPC->SetControlRotation(NewRotation);

            if (bShowDebugInfo && bFrameToggle)
            {
                UE_LOG(LogTemp, Log, TEXT("ControlRotationComponent: Blending (Alpha: %.3f, Time: %.2f/%.2f)"),
                    Alpha, ElapsedTime, BlendDuration);
            }
        }
        else
        {
            // Blend complete
            OwnerPC->SetControlRotation(BlendTargetRotation);
            bIsBlending = false;

            if (bShowDebugInfo)
            {
                UE_LOG(LogTemp, Log, TEXT("ControlRotationComponent: Blend complete. Final rotation: %s"),
                    *BlendTargetRotation.ToString());
            }
        }
    }
    else
    {
        if (!bPreserveMovementDirection)
        {
            AlignControlRotationToCamera();
        }
    }
}

UCameraComponent* UControlRotationComponent::GetViewTargetCameraComponent() const
{
    // ==================================================================================
    // GetViewTargetCameraComponent
    // - Gets the PlayerController's current view target
    // - Finds and returns its camera component
    // - Returns nullptr when no valid controller or view target exists
    // ==================================================================================

    if (!OwnerPC)
        return nullptr;

    AActor* ViewTarget = OwnerPC->GetViewTarget();
    if (!ViewTarget)
        return nullptr;

    return ViewTarget->FindComponentByClass<UCameraComponent>();
}