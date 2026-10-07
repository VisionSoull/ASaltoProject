// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LogicGateBase.generated.h"

class AActivableBase;
class InteractableBroadcaster;

UENUM(BlueprintType)
enum class ELogicGateMode : uint8
{
	TriggerOnce UMETA(DisplayName = "Trigger Once"),
	Continuous UMETA(DisplayName = "Continuous Monitor"),
	Toggle UMETA(DisplayName = "Toggle")
};

/**
 * Base class for all logic gates
 * Listens to InteractableBroadcasters and activates targets based on logic conditions
 * Derived classes override AreAllConditionsMet() for specific gate logic (AND, OR, XOR, etc.)
 */
UCLASS(Abstract)
class ASALTOPROJECT_API ALogicGateBase : public AActor
{
	GENERATED_BODY()

protected:
	/** Broadcasters that must meet their conditions */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic Gate|Inputs")
	TSet<AInteractableBroadcaster*> RequiredBroadcasters;

	/** Activables that must be active */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic Gate|Inputs")
	TSet<AActivableBase*> RequiredActivables;

	/** Activables to activate when all conditions are met */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic Gate|Outputs")
	TSet<AActivableBase*> TargetActivables;

	/** How this gate responds to condition changes */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic Gate")
	ELogicGateMode GateMode = ELogicGateMode::TriggerOnce;

	/** For Toggle mode: reset conditions after triggering? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic Gate", meta = (EditCondition = "GateMode == ELogicGateMode::Toggle", EditConditionHides))
	bool bResetAfterToggle = true;

	/** Track current state of broadcasters */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<AInteractableBroadcaster*, bool> BroadcasterStates;

	bool bHasTriggered = false;

public:
	ALogicGateBase();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Called when a broadcaster's state changes - MUST be UFUNCTION for AddDynamic */
	UFUNCTION()
	void OnBroadcasterStateChanged(bool bIsActive, AInteractableBroadcaster* Broadcaster);

	/** Called when an activable changes state - MUST be UFUNCTION for AddDynamic */
	UFUNCTION()
	void OnActivableStateChanged(bool bNewState, AActivableBase* Activable);

	/** 
	 * Blueprint event called when broadcaster state changes
	 * Use this to react to broadcaster changes in Blueprint (sounds, VFX, etc.)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Logic Gate", meta = (DisplayName = "On Broadcaster State Changed"))
	void BP_OnBroadcasterStateChanged(bool bIsActive, AInteractableBroadcaster* Broadcaster);

	/** 
	 * Blueprint event called when activable state changes
	 * Use this to react to activable changes in Blueprint (sounds, VFX, etc.)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Logic Gate", meta = (DisplayName = "On Activable State Changed"))
	void BP_OnActivableStateChanged(bool bIsActive, AActivableBase* Activable);

	/** 
	 * Check if conditions are met for this specific gate type
	 * OVERRIDE THIS in derived classes (AND, OR, XOR, etc.)
	 */
	UFUNCTION(BlueprintPure, Category = "Logic Gate")
	virtual bool AreAllConditionsMet() const PURE_VIRTUAL(ALogicGateBase::AreAllConditionsMet, return false;);

	/** Common logic for handling condition changes */
	void EvaluateConditions();

	/** Activate all target activables */
	void ActivateTargets();

	/** Deactivate all target activables */
	void DeactivateTargets();

	/** Toggle all target activables */
	void ToggleTargets();

public:
	/** Manually reset the gate (allows re-triggering) */
	UFUNCTION(BlueprintCallable, Category = "Logic Gate")
	void ResetGate();

	/** Get current gate state */
	UFUNCTION(BlueprintPure, Category = "Logic Gate")
	bool HasTriggered() const { return bHasTriggered; }
};