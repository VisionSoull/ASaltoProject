// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractableCommon.h"
#include "GameFramework/Character.h"
#include "../Settings/InteractionSettings.h"

AInteractableCommon::AInteractableCommon()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AInteractableCommon::BeginPlay()
{
	Super::BeginPlay();
}

// ==================== IInteractable Interface ====================

void AInteractableCommon::BeginInteract_Implementation(ACharacter* Character)
{
	// Always called on server via InteractionComponent's Server RPC
	if (!CanInteract_Implementation(Character))
	{
		return;
	}

	switch (InteractionType)
	{
	case EInteractionType::Instant:
		ExecuteInteraction(Character);
		break;

	case EInteractionType::Hold:
		HoldingCharacter = Character;
		LastInteractingCharacter = Character;
		bHoldCompleted = false;
		GetWorldTimerManager().SetTimer(HoldTimer, this, &AInteractableCommon::OnHoldCompleted, RequiredHoldDuration, false);

		// Trigger hold started feedback via replication
		HoldFeedbackState = EHoldFeedbackState::Holding;
		// Server plays feedback locally (OnRep doesn't fire on server)
		OnHoldStarted_Feedback(Character);
		break;

	case EInteractionType::Toggle:
		ExecuteInteraction(Character);
		bIsToggledOn = !bIsToggledOn;
		OnToggleChanged_Server_Implementation(bIsToggledOn, Character);
		// Server plays toggle feedback locally (clients get it via OnRep_IsToggledOn)
		OnToggleChanged_Feedback(bIsToggledOn, Character);
		break;
	}
}

void AInteractableCommon::EndInteract_Implementation(ACharacter* Character)
{
	// Always called on server via InteractionComponent's Server RPC
	if (InteractionType == EInteractionType::Hold && HoldingCharacter == Character)
	{
		if (bHoldCompleted)
		{
			// Hold was completed, now releasing

			if (bDeactivateOnRelease)
			{
				// Trigger deactivation logic
				bIsToggledOn = false;
				OnHoldReleased_Server_Implementation(Character);
			}

			// Update feedback state
			LastInteractingCharacter = Character;
			HoldFeedbackState = EHoldFeedbackState::Idle;

			// Server plays feedback locally
			// Server plays feedback locally
			OnHoldReleased_Feedback(Character);
		}
		else
		{
			// Released before completion - just cancel
			GetWorldTimerManager().ClearTimer(HoldTimer);

			LastInteractingCharacter = Character;
			HoldFeedbackState = EHoldFeedbackState::Cancelled;

			// Server plays feedback locally
			OnHoldCancelled_Feedback(Character);
		}

		// Reset state
		HoldingCharacter = nullptr;
		bHoldCompleted = false;
	}
}

void AInteractableCommon::BeginFocus_Implementation(ACharacter* Character, UPrimitiveComponent* FocusedComponent)
{
	Super::BeginFocus_Implementation(Character, FocusedComponent);
}

bool AInteractableCommon::CanInteract_Implementation(ACharacter* Character) const
{
	if (!Super::CanInteract_Implementation(Character))
	{
		return false;
	}

	if (!bIsActive)
	{
		return false;
	}

	if (bIsBusy)
	{
		return false;
	}

	if (bIsOnCooldown)
	{
		return false;
	}

	if (bHasBeenUsed && !bCanBeUsedMultipleTimes)
	{
		return false;
	}

	if (HoldingCharacter != nullptr && HoldingCharacter != Character)
	{
		return false;
	}

	return true;
}

// ==================== Internal Logic ====================

void AInteractableCommon::OnHoldCompleted()
{
	if (HoldingCharacter)
	{
		bHoldCompleted = true;
		bIsToggledOn = true;

		// Update feedback state
		HoldFeedbackState = EHoldFeedbackState::Completed;

		// Server plays feedback locally
		OnHoldCompleted_Feedback(HoldingCharacter);

		// Execute activation logic
		ExecuteInteraction(HoldingCharacter);

		// Not clearing HoldingCharacter to keep tracking who's holding
		// This allows EndInteract to know if the hold was completed
	}
}

void AInteractableCommon::OnCooldownExpired()
{
	bIsOnCooldown = false;
	LastInteractingCharacter = nullptr;
	BP_OnCooldownExpired();

	// Server calls OnRep manually since OnRep doesn't fire on authority
	OnRep_IsOnCooldown();
}

void AInteractableCommon::ExecuteInteraction(ACharacter* Character)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("ExecuteInteraction"));

	if (!bCanBeUsedMultipleTimes)
	{
		bHasBeenUsed = true;
	}

	// Trigger feedback via replication
	LastInteractingCharacter = Character;

	// Server plays feedback locally
	OnInteractionExecuted_Feedback(Character);

	// Server-only logic
	OnInteractionExecuted_Server_Implementation(Character);

	// Broadcast delegate for any listeners
	OnInteracted.Broadcast(this, Character);

	// Handle cooldown
	const float coolDown = GetCooldownDuration();
	if (coolDown > 0.0f && InteractionType != EInteractionType::Hold)
	{
		bIsOnCooldown = true;
		// Server calls OnRep manually
		OnRep_IsOnCooldown();
		GetWorldTimerManager().SetTimer(CooldownTimer, this, &AInteractableCommon::OnCooldownExpired, coolDown, false);
	}
}

// ==================== Server-Only Logic ====================

void AInteractableCommon::OnInteractionExecuted_Server_Implementation(ACharacter* Character)
{
	// Base implementation empty - derived classes override for specific logic
}

void AInteractableCommon::OnToggleChanged_Server_Implementation(bool bNewState, ACharacter* Character)
{
	// Base implementation empty - derived classes override for specific logic
}

void AInteractableCommon::OnHoldReleased_Server_Implementation(ACharacter* Character)
{
	// Base implementation empty - derived classes override for deactivation logic
}

// ==================== OnRep Functions ====================

void AInteractableCommon::OnRep_LastInteractingCharacter()
{
	if (LastInteractingCharacter)
	{
		// Client received interaction event - play feedback locally
		OnInteractionExecuted_Feedback(LastInteractingCharacter);
	}
}

void AInteractableCommon::OnRep_HoldFeedbackState()
{
	// Client received hold state change - play appropriate feedback
	switch (HoldFeedbackState)
	{
	case EHoldFeedbackState::Holding:
		OnHoldStarted_Feedback(LastInteractingCharacter);
		break;
	case EHoldFeedbackState::Completed:
		OnHoldCompleted_Feedback(LastInteractingCharacter);
		break;
	case EHoldFeedbackState::Cancelled:
		OnHoldCancelled_Feedback(LastInteractingCharacter);
		break;
	case EHoldFeedbackState::Idle:
		// No feedback needed for idle state
		break;
	}
}

void AInteractableCommon::OnRep_IsActive()
{
	UpdateFocusVisual();
	OnActivableStateChanged_Feedback(bIsActive);
}

void AInteractableCommon::OnRep_IsToggledOn()
{
	UpdateFocusVisual();

	// Client received toggle change - play feedback
	OnToggleChanged_Feedback(bIsToggledOn, LastInteractingCharacter);
}

void AInteractableCommon::OnRep_IsOnCooldown()
{
	UpdateFocusVisual();
}

void AInteractableCommon::OnRep_IsBusy()
{
	UpdateFocusVisual();
}

void AInteractableCommon::OnRep_HoldingCharacter()
{
	UpdateFocusVisual();
}

// ==================== IActivable Interface ====================

void AInteractableCommon::SetActive_Implementation(bool bActive)
{
	if (bIsActive == bActive)
	{
		return;
	}

	bIsActive = bActive;

	// Server calls OnRep manually
	OnRep_IsActive();

	if (bIsActive)
	{
		OnActivated_Implementation();
	}
	else
	{
		OnDeactivated_Implementation();
	}
}

bool AInteractableCommon::IsActive_Implementation() const
{
	return bIsActive;
}

bool AInteractableCommon::CanActivate_Implementation() const
{
	return true;
}

void AInteractableCommon::Toggle_Implementation()
{
	SetActive_Implementation(!bIsActive);
}

void AInteractableCommon::OnActivated_Implementation()
{
	// Base implementation empty - derived classes can override
}

void AInteractableCommon::OnDeactivated_Implementation()
{
	// Base implementation empty - derived classes can override
}

// ==================== Public API ====================

void AInteractableCommon::SetBusy(bool bNewBusy)
{
	bIsBusy = bNewBusy;
	OnRep_IsBusy();
}

void AInteractableCommon::SetToggledOn(bool bNewState)
{
	if (bIsToggledOn != bNewState)
	{
		bIsToggledOn = bNewState;
		OnToggleChanged_Server_Implementation(bIsToggledOn, nullptr);

		// Server plays feedback locally, clients get it via OnRep_IsToggledOn
		OnToggleChanged_Feedback(bIsToggledOn, nullptr);
	}
}

void AInteractableCommon::ResetInteractable()
{
	bHasBeenUsed = false;
	bIsToggledOn = false;
	bIsOnCooldown = false;
	bHoldCompleted = false;
	HoldingCharacter = nullptr;
	bIsBusy = false;
	HoldFeedbackState = EHoldFeedbackState::Idle;
	LastInteractingCharacter = nullptr;

	GetWorldTimerManager().ClearTimer(HoldTimer);
	GetWorldTimerManager().ClearTimer(CooldownTimer);

	UpdateFocusVisual();
}

float AInteractableCommon::GetHoldProgress() const
{
	if (InteractionType != EInteractionType::Hold || !HoldTimer.IsValid())
	{
		return 0.0f;
	}

	float Elapsed = GetWorldTimerManager().GetTimerElapsed(HoldTimer);
	return FMath::Clamp(Elapsed / RequiredHoldDuration, 0.0f, 1.0f);
}

float AInteractableCommon::GetRequiredHoldDuration()
{
	const UInteractionSettings* Settings = GetDefault<UInteractionSettings>();
	RequiredHoldDuration = bUseCustomHoldDuration ? CustomHoldDuration : Settings->DefaultHoldDuration;
	return RequiredHoldDuration;
}

float AInteractableCommon::GetCooldownDuration()
{
	const UInteractionSettings* Settings = GetDefault<UInteractionSettings>();
	CooldownDuration = bUseCustomCooldown ? CustomCooldownDuration : Settings->DefaultCooldown;
	return CooldownDuration;
}