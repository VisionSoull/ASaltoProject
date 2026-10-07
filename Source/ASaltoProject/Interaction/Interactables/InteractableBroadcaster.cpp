// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractableBroadcaster.h"

void AInteractableBroadcaster::BroadcastState(bool bNewState)
{
	OnInteractionBroadcast.Broadcast(bNewState, this);
}

void AInteractableBroadcaster::OnInteractionExecuted_Server_Implementation(ACharacter* Character)
{
	Super::OnInteractionExecuted_Server_Implementation(Character);

	if (!bAutoBroadcast)
	{
		return;
	}

	// Broadcast based on interaction type
	switch (InteractionType)
	{
	case EInteractionType::Instant:
		// Instant always broadcasts "true" (means "was pressed")
		BroadcastState(true);
		break;

	case EInteractionType::Hold:
		// Hold completion broadcasts "true"
		BroadcastState(bIsToggledOn);
		break;

	case EInteractionType::Toggle:
		// Toggle state is handled in OnToggleChanged_Server
		break;
	}
}

void AInteractableBroadcaster::OnHoldReleased_Server_Implementation(ACharacter* Character)
{
	Super::OnHoldReleased_Server_Implementation(Character);

	if (bAutoBroadcast)
	{
		BroadcastState(bIsToggledOn);
	}
}

void AInteractableBroadcaster::OnToggleChanged_Server_Implementation(bool bNewState, ACharacter* Character)
{
	Super::OnToggleChanged_Server_Implementation(bNewState, Character);

	if (bAutoBroadcast)
	{
		BroadcastState(bNewState);
	}
}