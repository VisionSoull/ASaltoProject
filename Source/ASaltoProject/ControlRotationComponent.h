#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "ControlRotationComponent.generated.h"

UCLASS(ClassGroup=(Camera), meta=(BlueprintSpawnableComponent))
class ASALTOPROJECT_API UControlRotationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UControlRotationComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
        FActorComponentTickFunction* ThisTickFunction) override;

protected:
    virtual void BeginPlay() override;

    // ==================================================================================
    // SETTINGS
    // ==================================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control Rotation")
    bool bEnableControlRotationAlignment = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebugInfo = false;

    // ==================================================================================
    // BLUEPRINT INTERFACE
    // ==================================================================================

    // Call this from Blueprint when using SetViewTargetWithBlend to sync control rotation with the blend
    UFUNCTION(BlueprintCallable, Category = "Control Rotation")
    void NotifyCameraBlend(float InBlendDuration);

private:
    // ==================================================================================
    // INTERNAL STATE
    // ==================================================================================

    UPROPERTY()
    APlayerController* OwnerPC = nullptr;

    AActor* LastViewTarget = nullptr;
    bool bFrameToggle = false;

    // Blend synchronization state
    bool bIsBlending = false;
    float BlendStartTime = 0.0f;
    float BlendDuration = 0.0f;
    FRotator BlendStartRotation = FRotator::ZeroRotator;
    FRotator BlendTargetRotation = FRotator::ZeroRotator;

    // ==================================================================================
    // INTERNAL FUNCTIONS
    // ==================================================================================

    void UpdateControlRotation(float DeltaTime);
    void OnViewTargetChanged();
    
    UCameraComponent* GetViewTargetCameraComponent() const;
    void StartBlendSync(float Duration);
    float EstimateBlendDuration() const;
};