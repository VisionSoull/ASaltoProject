// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractableActivator.h"
#include "../Activables/Activable.h"

void AInteractableActivator::OnInteractionExecuted_Server_Implementation(ACharacter* Character)
{
	Super::OnInteractionExecuted_Server_Implementation(Character);

	if (bAutoActivateTargets)
	{
		// For non-Toggle interactions (Instant, Hold, UI), always activate
		// Toggle interactions use OnToggleChanged_Server instead
		if (InteractionType != EInteractionType::Toggle)
		{
			ActivateTargets();
		}
	}
}

void AInteractableActivator::OnToggleChanged_Server_Implementation(bool bNewState, ACharacter* Character)
{
	Super::OnToggleChanged_Server_Implementation(bNewState, Character);

	if (bAutoActivateTargets)
	{
		// For Toggle interactions, activate/deactivate based on state
		if (bNewState)
		{
			ActivateTargets();
		}
		else
		{
			DeactivateTargets();
		}
	}
}

void AInteractableActivator::OnHoldReleased_Server_Implementation(ACharacter* Character)
{
	Super::OnHoldReleased_Server_Implementation(Character);

	// Only deactivate if configured to do so
	if (bAutoActivateTargets && bDeactivateOnRelease)
	{
		DeactivateTargets();
	}
}

void AInteractableActivator::ActivateTargets()
{
	for (AActor* Target : TargetActivables)
	{
		if (Target && Target->GetClass()->ImplementsInterface(UActivable::StaticClass()))
		{
			IActivable::Execute_SetActive(Target, true);
		}
	}
}

void AInteractableActivator::DeactivateTargets()
{
	for (AActor* Target : TargetActivables)
	{
		if (Target && Target->GetClass()->ImplementsInterface(UActivable::StaticClass()))
		{
			IActivable::Execute_SetActive(Target, false);
		}
	}
}

void AInteractableActivator::ToggleTargets()
{
	for (AActor* Target : TargetActivables)
	{
		if (Target && Target->GetClass()->ImplementsInterface(UActivable::StaticClass()))
		{
			IActivable::Execute_Toggle(Target);
		}
	}
}