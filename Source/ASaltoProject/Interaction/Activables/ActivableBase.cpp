// Fill out your copyright notice in the Description page of Project Settings.

#include "ActivableBase.h"

AActivableBase::AActivableBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AActivableBase::BeginPlay()
{
	Super::BeginPlay();
}

void AActivableBase::SetActive_Implementation(bool bActive)
{
	// Check if we can change state
	if (!IActivable::Execute_CanActivate(this))
	{
		return;
	}

	// Don't do anything if already in target state
	if (bIsActive == bActive)
	{
		return;
	}

	// Update state
	bIsActive = bActive;

	// Mark as used if this is the first activation
	if (bActive && !bHasBeenActivated)
	{
		bHasBeenActivated = true;
	}

	OnActivationStateChanged.Broadcast(bIsActive, this);

	// Fire appropriate events
	if (bIsActive)
	{
		IActivable::Execute_OnActivated(this);
	}
	else
	{
		IActivable::Execute_OnDeactivated(this);
	}

	// Start cooldown if configured
	if (ActivationCooldown > 0.0f)
	{
		bIsOnCooldown = true;
		GetWorldTimerManager().SetTimer(CooldownTimer, this, &AActivableBase::OnCooldownExpired, ActivationCooldown, false);
	}
}

bool AActivableBase::IsActive_Implementation() const
{
	return bIsActive;
}

bool AActivableBase::CanActivate_Implementation() const
{
	if (bIsBusy)
	{
		return false;
	}

	if (bIsOnCooldown)
	{
		return false;
	}

	if (bHasBeenActivated && !bCanToggleMultipleTimes)
	{
		return false;
	}

	return true;
}

void AActivableBase::Toggle_Implementation()
{
	IActivable::Execute_SetActive(this, !bIsActive);
}

void AActivableBase::OnActivated_Implementation()
{
	// Default: empty
	// Blueprint overrides this for specific behavior
}

void AActivableBase::OnDeactivated_Implementation()
{
	// Default: empty
	// Blueprint overrides this for specific behavior
}

void AActivableBase::OnCooldownExpired()
{
	bIsOnCooldown = false;
}

void AActivableBase::SetBusy(bool bNewBusy)
{
	bIsBusy = bNewBusy;
}

void AActivableBase::ResetActivable()
{
	bIsActive = false;
	bHasBeenActivated = false;
	bIsOnCooldown = false;
	bIsBusy = false;

	GetWorldTimerManager().ClearTimer(CooldownTimer);
}

void AActivableBase::SetCooldownActive(bool bActive)
{
	if (bActive)
	{
		if (ActivationCooldown > 0.0f)
		{
			bIsOnCooldown = true;
			GetWorldTimerManager().SetTimer(CooldownTimer, this, &AActivableBase::OnCooldownExpired, ActivationCooldown, false);
		}
	}
	else
	{
		bIsOnCooldown = false;
		GetWorldTimerManager().ClearTimer(CooldownTimer);
	}
}