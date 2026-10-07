// Fill out your copyright notice in the Description page of Project Settings.

#include "LogicGateAND.h"
#include "../Interactables/InteractableBroadcaster.h"
#include "../Activables/Activable.h"
#include "../Activables/ActivableBase.h"

bool ALogicGateAND::AreAllConditionsMet() const
{
	// Check all required broadcasters are in active state
	if (!RequiredBroadcasters.IsEmpty())
	{
		for (const AInteractableBroadcaster* Broadcaster : RequiredBroadcasters)
		{
			if (!Broadcaster)
			{
				return false;
			}

			const bool* StatePtr = BroadcasterStates.Find(Broadcaster);
			if (!StatePtr || !(*StatePtr))
			{
				return false; // Broadcaster not active
			}
		}
	}

	// Check all required activables are currently active
	if(!RequiredActivables.IsEmpty())
	{
		for (AActivableBase* Activable : RequiredActivables)
		{
			if (!Activable)
			{
				return false;
			}

			// Cast to UObject* for interface Execute function
			if (!IActivable::Execute_IsActive(Cast<UObject>(Activable)))
			{
				return false;
			}
		}
	}

	return true; // All conditions met!
}