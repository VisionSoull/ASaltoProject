#include "ControlRotationComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
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
    // Cache the owning PlayerController and the initial view target.
    // Optionally output debug info when bShowDebugInfo is enabled.
    // ==================================================================================

    OwnerPC = Cast<APlayerController>(GetOwner());

    if (!OwnerPC)
    {
        UE_LOG(LogTemp, Error, TEXT("ControlRotationComponent: Owner is not a PlayerController! Component will not function."));
        SetComponentTickEnabled(false);
        return;
    }

    LastViewTarget = OwnerPC->GetViewTarget();

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
    // Detects view target changes and updates control rotation alignment.
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

    // Update control rotation alignment
    UpdateControlRotation(DeltaTime);
}

void UControlRotationComponent::OnViewTargetChanged()
{
    // ==================================================================================
    // OnViewTargetChanged
    // Called when the PlayerController's view target changes. If no camera blend
    // is active, immediately align the control rotation to the new camera's
    // forward direction (yaw only). If a blend is active, synchronization will
    // happen over time via NotifyCameraBlend.
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

    // If not currently blending (no blend was notified), snap immediately
    if (!bIsBlending)
    {
        UCameraComponent* CameraComp = GetViewTargetCameraComponent();
        if (CameraComp)
        {
            FRotator CameraForward = CameraComp->GetForwardVector().Rotation();
            CameraForward.Pitch = 0.0f;
            CameraForward.Roll = 0.0f;
            OwnerPC->SetControlRotation(CameraForward);

            if (bShowDebugInfo)
            {
                UE_LOG(LogTemp, Log, TEXT("  No active blend - instant snap to: %s"), *CameraForward.ToString());
            }
        }
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
    // Called when a camera blend/transition starts. If the provided duration is
    // <= 0, perform an immediate alignment. Otherwise initiate a timed
    // synchronization that will interpolate control rotation over the duration.
    // ==================================================================================

    if (!OwnerPC || !bEnableControlRotationAlignment)
        return;

    if (InBlendDuration <= 0.0f)
    {
        // Instant transition
        UCameraComponent* CameraComp = GetViewTargetCameraComponent();
        if (CameraComp)
        {
            FRotator CameraForward = CameraComp->GetForwardVector().Rotation();
            CameraForward.Pitch = 0.0f;
            CameraForward.Roll = 0.0f;
            OwnerPC->SetControlRotation(CameraForward);
        }
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
    // Initialize blend synchronization state. Record start time, duration, and
    // the start/target rotations used for interpolation during UpdateControlRotation.
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

    // Target rotation from new camera's forward vector (yaw only)
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
    // Interpolate the PlayerController's control rotation during an active
    // camera blend according to the same easing curve used by the camera
    // system. When not blending, keep control rotation aligned to the
    // view target camera's forward direction (yaw only).
    // ==================================================================================

    if (!OwnerPC)
        return;

    UCameraComponent* CameraComp = GetViewTargetCameraComponent();
    if (!CameraComp)
        return;

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
        // Not blending - maintain alignment with camera
        FRotator CameraForward = CameraComp->GetForwardVector().Rotation();
        CameraForward.Pitch = 0.0f;
        CameraForward.Roll = 0.0f;

        OwnerPC->SetControlRotation(CameraForward);
    }
}

UCameraComponent* UControlRotationComponent::GetViewTargetCameraComponent() const
{
    if (!OwnerPC)
        return nullptr;

    AActor* ViewTarget = OwnerPC->GetViewTarget();
    if (!ViewTarget)
        return nullptr;

    return ViewTarget->FindComponentByClass<UCameraComponent>();
}