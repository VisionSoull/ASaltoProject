#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "FranciscoTypes.h"
#include "EmotionalStateComponent.generated.h"

class AFranciscoCharacter;
class UProximityComponent;

// Called when the panic turns on or off
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPanicLatchChanged, bool, bPanicked);

USTRUCT(BlueprintType)
struct FStressSourceData
{
    GENERATED_BODY()

    // Stress at the start of the segment
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float BaseStress = 0.0f;

    // Time when the segment started
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float SegmentStartTime = 0.0f;

    // Stress change per second
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float NetRate = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxStress = 100.0f;

    // Stress at a given time
    float Evaluate(float Now) const
    {
        return FMath::Clamp(BaseStress + NetRate * (Now - SegmentStartTime), 0.0f, MaxStress);
    }
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ASALTOPROJECT_API UEmotionalStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEmotionalStateComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    // Stress configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stress System", meta = (AllowPrivateAccess = "true"))
    float MaxTotalStress;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stress System", meta = (AllowPrivateAccess = "true"))
    float DistanceStressMultiplier;

    // Stress change per second in each zone
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stress Rates", meta = (AllowPrivateAccess = "true"))
    float ComfortZoneRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stress Rates", meta = (AllowPrivateAccess = "true"))
    float NormalZoneRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stress Rates", meta = (AllowPrivateAccess = "true"))
    float WarningZoneRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stress Rates", meta = (AllowPrivateAccess = "true"))
    float DangerZoneRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stress Rates", meta = (AllowPrivateAccess = "true"))
    float PanicZoneRate;

    // Emotional thresholds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotional Thresholds", meta = (AllowPrivateAccess = "true"))
    float NervousThreshold;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotional Thresholds", meta = (AllowPrivateAccess = "true"))
    float AnxiousThreshold;

    // Panic starts at this value
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotional Thresholds", meta = (AllowPrivateAccess = "true"))
    float PanicThreshold;

    // Panic ends at this value
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotional Thresholds", meta = (AllowPrivateAccess = "true"))
    float PanicRecoverThreshold;

    // Debug
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (AllowPrivateAccess = "true"))
    bool bDebugStressLevels;

    // Current state
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status", meta = (AllowPrivateAccess = "true"))
    float TotalStress;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status", meta = (AllowPrivateAccess = "true"))
    EEmotionalState CurrentEmotionalState;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status", meta = (AllowPrivateAccess = "true"))
    bool bPanicLatched;

    // Stress sources
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stress Sources", meta = (AllowPrivateAccess = "true"))
    TMap<EStressSource, FStressSourceData> StressSources;

    // References
    UPROPERTY()
    AFranciscoCharacter* OwnerCharacter;

    UPROPERTY()
    UProximityComponent* ProximityComponent;

    // Internal
    float LastStress;
    EEmotionalState LastEmotionalState;
    FTimerHandle EvaluationTimer;
    bool bPanicEnabled = true;

public:
    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnStressChanged OnStressChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPanicLatchChanged OnPanicLatchChanged;

    // Public interface
    UFUNCTION(BlueprintCallable, Category = "Emotional State")
    void AddStress(float Amount, EStressSource Source);

    UFUNCTION(BlueprintCallable, Category = "Emotional State")
    void ReduceStress(float Amount, EStressSource Source);

    UFUNCTION(BlueprintCallable, Category = "Emotional State")
    void ClearStressSource(EStressSource Source);

    UFUNCTION(BlueprintCallable, Category = "Emotional State")
    float GetTotalStress() const;

    UFUNCTION(BlueprintCallable, Category = "Emotional State")
    EEmotionalState GetEmotionalState() const { return CurrentEmotionalState; }

    UFUNCTION(BlueprintCallable, Category = "Emotional State")
    float GetStressFromSource(EStressSource Source) const;

    UFUNCTION(BlueprintCallable, Category = "Emotional State")
    bool IsPanicLatched() const { return bPanicLatched; }

    UFUNCTION(BlueprintCallable, Category = "Emotional State")
    bool IsInPanicState() const { return bPanicLatched; }

    // Turns the panic system on or off
    UFUNCTION(BlueprintCallable, Category = "Emotional State")
    void SetPanicEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Emotional State")
    bool IsPanicEnabled() const { return bPanicEnabled; }

protected:
    float GetCurrentTime() const;
    float ComputeTotalStress(float Now) const;
    float ComputeTimeToNextEvent(float Now) const;
    void SetSourceRate(EStressSource Source, float NewRate);
    void ApplyZoneRate(EProximityZone Zone);
    void RefreshState();
    void ScheduleNextEvaluation();
    EEmotionalState CalculateEmotionalState() const;
    void DrawDebugInfo() const;

    // Listen to events
    UFUNCTION()
    void OnZoneChanged(EProximityZone NewZone);
};