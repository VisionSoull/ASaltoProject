// Fill out your copyright notice in the Description page of Project Settings.

#include "LogicGateBase.h"
#include "../Activables/ActivableBase.h"
#include "../Activables/Activable.h"
#include "../Interactables/InteractableBroadcaster.h"

ALogicGateBase::ALogicGateBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALogicGateBase::BeginPlay()
{
	Super::BeginPlay();

	// Subscribe to all required broadcasters
	for (AInteractableBroadcaster* Broadcaster : RequiredBroadcasters)
	{
		if (!Broadcaster)
		{
			UE_LOG(LogTemp, Warning, TEXT("LogicGate '%s' has null broadcaster"), *GetName());
			continue;
		}

		// Subscribe to broadcast delegate
		Broadcaster->OnInteractionBroadcast.AddDynamic(this, &ALogicGateBase::OnBroadcasterStateChanged);

		// Initialize state tracking with current state
		BroadcasterStates.Add(Broadcaster, Broadcaster->IsToggledOn());

		UE_LOG(LogTemp, Log, TEXT("LogicGate '%s': Subscribed to broadcaster '%s', initial state: %s"),
			*GetName(), *Broadcaster->GetName(), Broadcaster->IsToggledOn() ? TEXT("ACTIVE") : TEXT("INACTIVE"));
	}

	// Subscribe to all required activables
	for (AActivableBase* Activable : RequiredActivables)
	{
		if (Activable)
		{
			Activable->OnActivationStateChanged.AddDynamic(this, &ALogicGateBase::OnActivableStateChanged);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("LogicGate '%s' has null activable"), *GetName());
		}
	}

	// Check initial state
	if (AreAllConditionsMet())
	{
		UE_LOG(LogTemp, Warning, TEXT("LogicGate '%s': Initial conditions already met!"), *GetName());

		switch (GateMode)
		{
		case ELogicGateMode::TriggerOnce:
			ActivateTargets();
			bHasTriggered = true;
			break;

		case ELogicGateMode::Continuous:
			ActivateTargets();
			break;

		case ELogicGateMode::Toggle:
			ToggleTargets();
			bHasTriggered = true;
			break;
		}
	}
}

void ALogicGateBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unsubscribe from broadcasters
	for (AInteractableBroadcaster* Broadcaster : RequiredBroadcasters)
	{
		if (Broadcaster)
		{
			Broadcaster->OnInteractionBroadcast.RemoveDynamic(this, &ALogicGateBase::OnBroadcasterStateChanged);
		}
	}

	// Unsubscribe from activables
	for (AActivableBase* Activable : RequiredActivables)
	{
		if (Activable)
		{
			Activable->OnActivationStateChanged.RemoveDynamic(this, &ALogicGateBase::OnActivableStateChanged);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ALogicGateBase::OnBroadcasterStateChanged(bool bIsActive, AInteractableBroadcaster* Broadcaster)
{
	// Track broadcaster state change (CRITICAL - must always happen)
	BroadcasterStates.Add(Broadcaster, bIsActive);

	UE_LOG(LogTemp, Warning, TEXT("LogicGate '%s': Broadcaster '%s' state changed to %s"),
		*GetName(), *Broadcaster->GetName(), bIsActive ? TEXT("ACTIVE") : TEXT("INACTIVE"));

	// Fire Blueprint event (optional custom behavior)
	BP_OnBroadcasterStateChanged(bIsActive, Broadcaster);

	// Evaluate conditions
	EvaluateConditions();
}

void ALogicGateBase::OnActivableStateChanged(bool bNewState, AActivableBase* Activable)
{
	UE_LOG(LogTemp, Log, TEXT("LogicGate '%s': Activable '%s' state changed to %s"),
		*GetName(), *Activable->GetName(), bNewState ? TEXT("ACTIVE") : TEXT("INACTIVE"));

	// Fire Blueprint event (optional custom behavior)
	BP_OnActivableStateChanged(bNewState, Activable);

	// Evaluate conditions
	EvaluateConditions();
}

void ALogicGateBase::EvaluateConditions()
{
	bool bAllMet = AreAllConditionsMet();

	UE_LOG(LogTemp, Warning, TEXT("LogicGate '%s': EvaluateConditions - All met: %s"),
		*GetName(), bAllMet ? TEXT("YES") : TEXT("NO"));

	switch (GateMode)
	{
	case ELogicGateMode::TriggerOnce:
	{
		if (bAllMet && !bHasTriggered)
		{
			ActivateTargets();
			bHasTriggered = true;
			UE_LOG(LogTemp, Warning, TEXT("LogicGate '%s': TriggerOnce activated!"), *GetName());
		}
		break;
	}

	case ELogicGateMode::Continuous:
	{
		if (bAllMet)
		{
			ActivateTargets();
		}
		else
		{
			DeactivateTargets();
		}
		break;
	}

	case ELogicGateMode::Toggle:
	{
		if (bAllMet && !bHasTriggered)
		{
			ToggleTargets();
			bHasTriggered = true;
			UE_LOG(LogTemp, Warning, TEXT("LogicGate '%s': Toggle triggered!"), *GetName());
		}
		else if (!bAllMet && bHasTriggered && bResetAfterToggle)
		{
			bHasTriggered = false;
		}
		break;
	}
	}
}

void ALogicGateBase::ActivateTargets()
{
	UE_LOG(LogTemp, Warning, TEXT("LogicGate '%s': Activating %d targets"), *GetName(), TargetActivables.Num());

	for (AActivableBase* Target : TargetActivables)
	{
		if (Target && Target->Implements<UActivable>())
		{
			IActivable::Execute_SetActive(Cast<UObject>(Target), true);
			UE_LOG(LogTemp, Log, TEXT("  - Activated target: %s"), *Target->GetName());
		}
	}
}

void ALogicGateBase::DeactivateTargets()
{
	for (AActivableBase* Target : TargetActivables)
	{
		if (Target && Target->Implements<UActivable>())
		{
			IActivable::Execute_SetActive(Cast<UObject>(Target), false);
		}
	}
}

void ALogicGateBase::ToggleTargets()
{
	for (AActivableBase* Target : TargetActivables)
	{
		if (Target && Target->Implements<UActivable>())
		{
			IActivable::Execute_Toggle(Cast<UObject>(Target));
		}
	}
}

void ALogicGateBase::ResetGate()
{
	bHasTriggered = false;
	BroadcasterStates.Empty();

	// Re-initialize broadcaster states
	for (AInteractableBroadcaster* Broadcaster : RequiredBroadcasters)
	{
		if (Broadcaster)
		{
			BroadcasterStates.Add(Broadcaster, Broadcaster->IsActive());
		}
	}

	if (AreAllConditionsMet())
	{
		switch (GateMode)
		{
		case ELogicGateMode::TriggerOnce:
			ActivateTargets();
			bHasTriggered = true;
			break;

		case ELogicGateMode::Continuous:
			ActivateTargets();
			break;

		case ELogicGateMode::Toggle:
			ToggleTargets();
			bHasTriggered = true;
			break;
		}
	}
	else
	{
		DeactivateTargets();
	}

	UE_LOG(LogTemp, Log, TEXT("LogicGate '%s': Manual reset"), *GetName());
}