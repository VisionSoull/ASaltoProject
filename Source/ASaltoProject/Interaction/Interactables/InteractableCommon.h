// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractableBase.h"
#include "EInteractionType.h"
#include "../Activables/Activable.h"
#include "InteractableCommon.generated.h"

/**
 * Feedback state for hold interactions - replicated to trigger OnRep on clients
 */
UENUM(BlueprintType)
enum class EHoldFeedbackState : uint8
{
	Idle,
	Holding,
	Completed,
	Cancelled
};

/**
 * Common base class for interactables with timing logic (cooldowns, hold duration, busy state).
 * All interaction logic runs on server; clients route requests via InteractionComponent's Server RPCs.
 *
 * Feedback System:
 * - Server changes replicated state → OnRep fires on clients → clients play feedback locally
 * - Server also plays feedback locally (since OnRep doesn't fire on the machine that sets the value)
 *
 * Implements IActivable for external control (e.g., a lever could be disabled until fixed).
 */
UCLASS(Abstract, Blueprintable)
class ASALTOPROJECT_API AInteractableCommon : public AInteractableBase, public IActivable
{
	GENERATED_BODY()

protected:
	// ==================== Configuration ====================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	EInteractionType InteractionType = EInteractionType::Instant;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bUseCustomCooldown = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (EditCondition = "bUseCustomCooldown", ClampMin = 0.0, ClampMax = 10.0))
	float CustomCooldownDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (EditCondition = "InteractionType == EInteractionType::Hold"))
	bool bUseCustomHoldDuration = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (EditCondition = "bUseCustomHoldDuration && InteractionType == EInteractionType::Hold", ClampMin = 0.1, ClampMax = 10.0))
	float CustomHoldDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bCanBeUsedMultipleTimes = true;

	/** Should targets deactivate when hold is released? Enables "hold to keep active" functionality */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Hold", meta = (EditCondition = "InteractionType == EInteractionType::Hold"))
	bool bDeactivateOnRelease = false;

	// ==================== Core Replicated State ====================

	/** IActivable state - when false, this interactable is disabled and cannot be used */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Activable")
	bool bIsActive = true;

	/** Toggle interaction state - for Toggle type, is the switch on or off? */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Toggle")
	bool bIsToggledOn = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsBusy = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsOnCooldown = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bHasBeenUsed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Hold")
	bool bHoldCompleted = false;

	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	float CooldownDuration = 0.5f;

	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	float RequiredHoldDuration = 2.0f;

	FTimerHandle HoldTimer;
	FTimerHandle CooldownTimer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	ACharacter* HoldingCharacter = nullptr;

	// ==================== Feedback State (Replicated for OnRep-based feedback) ====================

	/** Hold feedback state - OnRep triggers hold start/complete/cancel feedback on clients */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Feedback")
	EHoldFeedbackState HoldFeedbackState = EHoldFeedbackState::Idle;

	/** Character who triggered the current interaction (replicated for feedback context) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Feedback")
	TObjectPtr<ACharacter> LastInteractingCharacter = nullptr;

public:
	AInteractableCommon();

protected:
	virtual void BeginPlay() override;

	// ==================== IInteractable Interface ====================

	virtual void BeginInteract_Implementation(ACharacter* Character) override;
	virtual void EndInteract_Implementation(ACharacter* Character) override;
	virtual void BeginFocus_Implementation(ACharacter* Character, UPrimitiveComponent* FocusedComponent) override;
	virtual bool CanInteract_Implementation(ACharacter* Character) const override;

	// ==================== IActivable Interface ====================

	virtual void SetActive_Implementation(bool bActive) override;
	virtual bool IsActive_Implementation() const override;
	virtual bool CanActivate_Implementation() const override;
	virtual void Toggle_Implementation() override;
	virtual void OnActivated_Implementation() override;
	virtual void OnDeactivated_Implementation() override;

	// ==================== Internal Logic ====================

	void OnHoldCompleted();
	void OnCooldownExpired();
	void ExecuteInteraction(ACharacter* Character);

	// ==================== Server-Only Logic (Override in Derived Classes) ====================

	/**
	 * Server-only: Called when interaction completes.
	 * Override this in derived classes (Broadcaster, Activator) for server logic.
	 * Do NOT call visual/audio feedback here - use the Feedback events instead.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction|Server")
	void OnInteractionExecuted_Server(ACharacter* Character);

	/**
	 * Server-only: Called when toggle state changes.
	 * Override this in derived classes for server logic.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction|Server")
	void OnToggleChanged_Server(bool bNewState, ACharacter* Character);

	/**
	 * Server-only: Called when hold is released after completion.
	 * Override this in derived classes for deactivation logic.
	 * Only called if bDeactivateOnRelease is true.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction|Server")
	void OnHoldReleased_Server(ACharacter* Character);

	// ==================== Feedback Events (Fire on ALL Machines via OnRep) ====================

	/**
	 * All machines: Visual/audio feedback when interaction completes.
	 * Called via OnRep_LastInteractingCharacter on clients, directly on server.
	 * Safe to call BroadcastState/ActivateTargets from here since they have authority checks.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnInteractionExecuted_Feedback(ACharacter* Character);

	/** All machines: Visual/audio feedback when hold interaction starts */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnHoldStarted_Feedback(ACharacter* Character);

	/** All machines: Visual/audio feedback when hold duration completes (lever fully down) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnHoldCompleted_Feedback(ACharacter* Character);

	/** All machines: Visual/audio feedback when hold is released after completion (lever returns up) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnHoldReleased_Feedback(ACharacter* Character);

	/** All machines: Visual/audio feedback when hold interaction is cancelled (released before completion) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnHoldCancelled_Feedback(ACharacter* Character);

	/** All machines: Visual/audio feedback when toggle state changes */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnToggleChanged_Feedback(bool bNewState, ACharacter* Character);

	/** All machines: Visual/audio feedback when IActivable state changes */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnActivableStateChanged_Feedback(bool bNewActiveState);

	/** Called when cooldown expires */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Feedback", meta = (DisplayName = "OnCooldownExpired"))
	void BP_OnCooldownExpired();

	// ==================== OnRep Functions (Fire on Clients) ====================

	UFUNCTION()
	void OnRep_IsActive();

	UFUNCTION()
	void OnRep_IsToggledOn();

	UFUNCTION()
	void OnRep_IsOnCooldown();

	UFUNCTION()
	void OnRep_IsBusy();

	UFUNCTION()
	void OnRep_HoldingCharacter();

	UFUNCTION()
	void OnRep_LastInteractingCharacter();

	UFUNCTION()
	void OnRep_HoldFeedbackState();

public:
	// ==================== Public API ====================

	UFUNCTION(BlueprintCallable, Category = "State")
	void SetBusy(bool bNewBusy);

	UFUNCTION(BlueprintPure, Category = "State")
	bool IsBusy() const { return bIsBusy; }

	UFUNCTION(BlueprintCallable, Category = "Interaction", meta = (BlueprintAuthorityOnly))
	void SetToggledOn(bool bNewState);

	UFUNCTION(BlueprintCallable, Category = "Interaction", meta = (BlueprintAuthorityOnly))
	void ResetInteractable();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	float GetHoldProgress() const;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	float GetRequiredHoldDuration();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	float GetCooldownDuration();

	UFUNCTION(BlueprintPure, Category = "State")
	bool IsToggledOn() const { return bIsToggledOn; }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	EInteractionType GetInteractionType() const { return InteractionType; }
};