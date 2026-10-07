#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LogicGateSequence.generated.h"

class AInteractableBroadcaster;
class AActivableBase;

/**
 * Sequence Logic Gate - validates broadcasters activated in specific order
 * Example: Button1 → Button2 → Button3 → Button4
 * 
 * Prerequisites: Activables that must be active BEFORE sequence can start
 * Sequence: Broadcasters that must be activated in this exact order
 * Targets: Activables to activate when sequence completes
 */
UCLASS()
class ASALTOPROJECT_API ALogicGateSequence : public AActor
{
	GENERATED_BODY()

protected:
	/** Activables that must be active before sequence can begin (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic Gate|Prerequisites")
	TSet<AActivableBase*> RequiredActivables;

	/** Ordered sequence of broadcasters - must activate in this exact order */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic Gate|Sequence")
	TArray<AInteractableBroadcaster*> OrderedBroadcasters;

	/** Activables to activate when sequence completes */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic Gate|Outputs")
	TSet<AActivableBase*> TargetActivables;

	/** Current step in sequence (0 = waiting for first) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Logic Gate|State")
	int32 CurrentStep = 0;

	/** Is sequence currently active? (false if prerequisites not met) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Logic Gate|State")
	bool bSequenceEnabled = false;

	/** Has sequence been completed? */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Logic Gate|State")
	bool bSequenceComplete = false;

public:
	ALogicGateSequence();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Check if all prerequisite activables are active */
	bool ArePrerequisitesMet() const;

	/** Called when a broadcaster state changes */
	UFUNCTION()
	void OnBroadcasterStateChanged(bool bIsActive, AInteractableBroadcaster* Broadcaster);

	/** Called when a prerequisite activable state changes */
	UFUNCTION()
	void OnPrerequisiteStateChanged(bool bIsActive, AActivableBase* Activable);

	/** Enable/disable sequence based on prerequisites */
	void UpdateSequenceEnabled();

	/** Reset sequence to beginning */
	void ResetSequence();

	/** Activate all target activables */
	void ActivateTargets();

	// Blueprint Events

	/** Called when correct step in sequence is activated */
	UFUNCTION(BlueprintImplementableEvent, Category = "Logic Gate|Sequence", meta = (DisplayName = "On Sequence Progress"))
	void BP_OnSequenceProgress(int32 Step, AInteractableBroadcaster* Broadcaster);

	/** Called when wrong broadcaster is pressed (resets sequence) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Logic Gate|Sequence", meta = (DisplayName = "On Sequence Reset"))
	void BP_OnSequenceReset(AInteractableBroadcaster* WrongBroadcaster);

	/** Called when full sequence completes */
	UFUNCTION(BlueprintImplementableEvent, Category = "Logic Gate|Sequence", meta = (DisplayName = "On Sequence Complete"))
	void BP_OnSequenceComplete();

	/** Called when sequence becomes enabled/disabled due to prerequisites */
	UFUNCTION(BlueprintImplementableEvent, Category = "Logic Gate|Sequence", meta = (DisplayName = "On Sequence Enabled Changed"))
	void BP_OnSequenceEnabledChanged(bool bEnabled);

public:
	/** Get current progress */
	UFUNCTION(BlueprintPure, Category = "Logic Gate|Sequence")
	int32 GetCurrentStep() const { return CurrentStep; }

	/** Get total steps needed */
	UFUNCTION(BlueprintPure, Category = "Logic Gate|Sequence")
	int32 GetTotalSteps() const { return OrderedBroadcasters.Num(); }

	/** Is sequence currently enabled? */
	UFUNCTION(BlueprintPure, Category = "Logic Gate|Sequence")
	bool IsSequenceEnabled() const { return bSequenceEnabled; }

	/** Has sequence been completed? */
	UFUNCTION(BlueprintPure, Category = "Logic Gate|Sequence")
	bool IsSequenceComplete() const { return bSequenceComplete; }

	/** Manually reset sequence */
	UFUNCTION(BlueprintCallable, Category = "Logic Gate|Sequence")
	void ManualReset();
};