#include "EmotionalStateComponent.h"
#include "FranciscoCharacter.h"
#include "ProximityComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"

UEmotionalStateComponent::UEmotionalStateComponent()
{
    // Only ticks for the debug text
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;

    // Stress configuration
    MaxTotalStress = 100.0f;

    // Stress multipliers
    DistanceStressMultiplier = 1.0f;

    // Stress change per second in each zone
    ComfortZoneRate = -20.0f;
    NormalZoneRate = -5.0f;
    WarningZoneRate = 5.0f;
    DangerZoneRate = 15.0f;
    PanicZoneRate = 30.0f;

    // Emotional thresholds
    NervousThreshold = 30.0f;
    AnxiousThreshold = 60.0f;
    PanicThreshold = 80.0f;
    PanicRecoverThreshold = 30.0f;

    // Debug
    bDebugStressLevels = false;

    // Initialize state
    TotalStress = 0.0f;
    CurrentEmotionalState = EEmotionalState::Calm;
    bPanicLatched = false;
    LastStress = 0.0f;
    LastEmotionalState = EEmotionalState::Calm;

    // Initialize stress sources
    StressSources.Add(EStressSource::Distance, FStressSourceData());

    OwnerCharacter = nullptr;
    ProximityComponent = nullptr;
}

void UEmotionalStateComponent::BeginPlay()
{
    Super::BeginPlay();

    // Get owner
    OwnerCharacter = Cast<AFranciscoCharacter>(GetOwner());

    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("EmotionalStateComponent: Owner is not AFranciscoCharacter!"));
        return;
    }

    // Make sure the Distance source exists
    if (!StressSources.Contains(EStressSource::Distance))
    {
        StressSources.Add(EStressSource::Distance, FStressSourceData());
    }

    // Reset all sources
    const float Now = GetCurrentTime();
    for (auto& Pair : StressSources)
    {
        Pair.Value.BaseStress = 0.0f;
        Pair.Value.SegmentStartTime = Now;
        Pair.Value.NetRate = 0.0f;
    }

    // Get proximity component
    ProximityComponent = OwnerCharacter->FindComponentByClass<UProximityComponent>();
    if (ProximityComponent)
    {
        ProximityComponent->OnZoneChanged.AddDynamic(this, &UEmotionalStateComponent::OnZoneChanged);
        ApplyZoneRate(ProximityComponent->GetCurrentZone());
        UE_LOG(LogTemp, Log, TEXT("EmotionalStateComponent: Registered with ProximityComponent"));
    }

    SetComponentTickEnabled(bDebugStressLevels);
    RefreshState();

    UE_LOG(LogTemp, Log, TEXT("EmotionalStateComponent initialized"));
}

void UEmotionalStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(EvaluationTimer);
    }

    Super::EndPlay(EndPlayReason);
}

void UEmotionalStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bDebugStressLevels)
    {
        DrawDebugInfo();
    }
}

float UEmotionalStateComponent::GetCurrentTime() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetTimeSeconds() : 0.0f;
}

float UEmotionalStateComponent::ComputeTotalStress(float Now) const
{
    float Sum = 0.0f;

    // Sum all stress sources
    for (const auto& Pair : StressSources)
    {
        Sum += Pair.Value.Evaluate(Now);
    }

    // Clamp to max
    return FMath::Clamp(Sum, 0.0f, MaxTotalStress);
}

float UEmotionalStateComponent::GetTotalStress() const
{
    return ComputeTotalStress(GetCurrentTime());
}

float UEmotionalStateComponent::GetStressFromSource(EStressSource Source) const
{
    if (const FStressSourceData* Data = StressSources.Find(Source))
    {
        return Data->Evaluate(GetCurrentTime());
    }
    return 0.0f;
}

// Seconds until the next threshold or cap is reached (-1 if none)
float UEmotionalStateComponent::ComputeTimeToNextEvent(float Now) const
{
    float Rate = 0.0f;
    float Best = TNumericLimits<float>::Max();

    for (const auto& Pair : StressSources)
    {
        const FStressSourceData& Data = Pair.Value;
        const float Cur = Data.Evaluate(Now);

        if (Data.NetRate > 0.0f && Cur < Data.MaxStress - 0.0001f)
        {
            Rate += Data.NetRate;
            Best = FMath::Min(Best, (Data.MaxStress - Cur) / Data.NetRate);
        }
        else if (Data.NetRate < 0.0f && Cur > 0.0001f)
        {
            Rate += Data.NetRate;
            Best = FMath::Min(Best, Cur / -Data.NetRate);
        }
    }

    const float Total = ComputeTotalStress(Now);

    // Already at the max
    if (Rate > 0.0f && Total >= MaxTotalStress - 0.0001f)
    {
        Rate = 0.0f;
    }

    // Stress is not changing
    if (FMath::Abs(Rate) < KINDA_SMALL_NUMBER)
    {
        return -1.0f;
    }

    if (Rate > 0.0f)
    {
        Best = FMath::Min(Best, (MaxTotalStress - Total) / Rate);
    }

    // Only check the panic value that matters right now
    const float Thresholds[3] = {
        NervousThreshold,
        AnxiousThreshold,
        bPanicLatched ? PanicRecoverThreshold : PanicThreshold
    };

    for (const float Threshold : Thresholds)
    {
        if (Rate > 0.0f && Threshold > Total + 0.001f)
        {
            Best = FMath::Min(Best, (Threshold - Total) / Rate);
        }
        else if (Rate < 0.0f && Threshold < Total - 0.001f)
        {
            Best = FMath::Min(Best, (Total - Threshold) / -Rate);
        }
    }

    return Best == TNumericLimits<float>::Max() ? -1.0f : Best;
}

void UEmotionalStateComponent::ScheduleNextEvaluation()
{
    UWorld* World = GetWorld();
    if (!World) return;

    FTimerManager& TimerManager = World->GetTimerManager();
    TimerManager.ClearTimer(EvaluationTimer);

    const float Dt = ComputeTimeToNextEvent(GetCurrentTime());
    if (Dt < 0.0f) return;

    // Small extra delay so the value is past the threshold
    const float Delay = FMath::Max(Dt + 0.02f, 0.05f);
    TimerManager.SetTimer(EvaluationTimer, this, &UEmotionalStateComponent::RefreshState, Delay, false);
}

void UEmotionalStateComponent::RefreshState()
{
    const float Now = GetCurrentTime();

    LastStress = TotalStress;
    TotalStress = ComputeTotalStress(Now);

    // Panic on at PanicThreshold, off at PanicRecoverThreshold
    const bool bWasLatched = bPanicLatched;
    if (!bPanicLatched && TotalStress >= PanicThreshold)
    {
        bPanicLatched = true;
    }
    else if (bPanicLatched && TotalStress <= PanicRecoverThreshold)
    {
        bPanicLatched = false;
    }

    LastEmotionalState = CurrentEmotionalState;
    CurrentEmotionalState = CalculateEmotionalState();

    // Log emotional state changes
    if (CurrentEmotionalState != LastEmotionalState)
    {
        UE_LOG(LogTemp, Log, TEXT("EmotionalState: Changed from %s to %s (Stress: %.1f)"),
            *UEnum::GetValueAsString(LastEmotionalState),
            *UEnum::GetValueAsString(CurrentEmotionalState),
            TotalStress);
    }

    // Broadcast if changed significantly
    if (FMath::Abs(TotalStress - LastStress) > 1.0f || CurrentEmotionalState != LastEmotionalState)
    {
        OnStressChanged.Broadcast(LastStress, TotalStress);
    }

    if (bPanicLatched != bWasLatched)
    {
        OnPanicLatchChanged.Broadcast(bPanicLatched);
    }

    ScheduleNextEvaluation();
}

EEmotionalState UEmotionalStateComponent::CalculateEmotionalState() const
{
    if (bPanicLatched)
        return EEmotionalState::Panicked;
    else if (TotalStress >= AnxiousThreshold)
        return EEmotionalState::Anxious;
    else if (TotalStress >= NervousThreshold)
        return EEmotionalState::Nervous;
    else
        return EEmotionalState::Calm;
}

// Change the rate of a source
void UEmotionalStateComponent::SetSourceRate(EStressSource Source, float NewRate)
{
    if (FStressSourceData* Data = StressSources.Find(Source))
    {
        const float Now = GetCurrentTime();
        Data->BaseStress = Data->Evaluate(Now);
        Data->SegmentStartTime = Now;
        Data->NetRate = NewRate;
    }
}

// Pick the rate for a zone
void UEmotionalStateComponent::ApplyZoneRate(EProximityZone Zone)
{
    if (!bPanicEnabled)
    {
        SetSourceRate(EStressSource::Distance, 0.0f);
        return;
    }

    float Rate = 0.0f;

    switch (Zone)
    {
    case EProximityZone::Comfort: Rate = ComfortZoneRate; break;
    case EProximityZone::Normal:  Rate = NormalZoneRate;  break;
    case EProximityZone::Warning: Rate = WarningZoneRate; break;
    case EProximityZone::Danger:  Rate = DangerZoneRate;  break;
    case EProximityZone::Panic:   Rate = PanicZoneRate;   break;
    }

    // Multiplier only affects stress gain
    if (Rate > 0.0f)
    {
        Rate *= DistanceStressMultiplier;
    }

    SetSourceRate(EStressSource::Distance, Rate);
}

void UEmotionalStateComponent::AddStress(float Amount, EStressSource Source)
{
    if (!bPanicEnabled) return;

    if (FStressSourceData* Data = StressSources.Find(Source))
    {
        const float Now = GetCurrentTime();
        Data->BaseStress = FMath::Clamp(Data->Evaluate(Now) + Amount, 0.0f, Data->MaxStress);
        Data->SegmentStartTime = Now;
        RefreshState();
    }
}

void UEmotionalStateComponent::ReduceStress(float Amount, EStressSource Source)
{
    AddStress(-Amount, Source);
}

void UEmotionalStateComponent::ClearStressSource(EStressSource Source)
{
    if (FStressSourceData* Data = StressSources.Find(Source))
    {
        Data->BaseStress = 0.0f;
        Data->SegmentStartTime = GetCurrentTime();
        RefreshState();
    }
}

void UEmotionalStateComponent::OnZoneChanged(EProximityZone NewZone)
{
    // React to zone changes
    ApplyZoneRate(NewZone);
    RefreshState();
}

void UEmotionalStateComponent::DrawDebugInfo() const
{
    if (!OwnerCharacter) return;

    FVector Location = OwnerCharacter->GetActorLocation() + FVector(0, 0, 150);

    // Color based on emotional state
    FColor TextColor = FColor::White;
    switch (CurrentEmotionalState)
    {
    case EEmotionalState::Calm:     TextColor = FColor::Green; break;
    case EEmotionalState::Nervous:  TextColor = FColor::Yellow; break;
    case EEmotionalState::Anxious:  TextColor = FColor::Orange; break;
    case EEmotionalState::Panicked: TextColor = FColor::Red; break;
    }

    // Build debug string
    FString DebugText = FString::Printf(TEXT("\nTotal Stress: %.1f / %.1f\nState: %s\nPanic: %s\n"),
        GetTotalStress(), MaxTotalStress,
        *UEnum::GetValueAsString(CurrentEmotionalState),
        bPanicLatched ? TEXT("ON") : TEXT("off"));

    DrawDebugString(GetWorld(), Location, DebugText, nullptr, TextColor, 0.0f, true);
}

void UEmotionalStateComponent::SetPanicEnabled(bool bEnabled)
{
    if (bPanicEnabled == bEnabled) return;
    bPanicEnabled = bEnabled;

    if (!bPanicEnabled)
    {
        // Reset all stress
        for (auto& Pair : StressSources)
        {
            Pair.Value.BaseStress = 0.0f;
            Pair.Value.SegmentStartTime = GetCurrentTime();
            Pair.Value.NetRate = 0.0f;
        }
    }
    else if (ProximityComponent)
    {
        // Start again from the current zone
        ApplyZoneRate(ProximityComponent->GetCurrentZone());
    }

    RefreshState();
}