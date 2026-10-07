#include "LogicGateSequence.h"
#include "../Interactables/InteractableBroadcaster.h"
#include "../Activables/ActivableBase.h"
#include "../Activables/Activable.h"

ALogicGateSequence::ALogicGateSequence()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALogicGateSequence::BeginPlay()
{
	Super::BeginPlay();

	// Subscribe to prerequisite activables
	for (AActivableBase* Activable : RequiredActivables)
	{
		if (Activable)
		{
			Activable->OnActivationStateChanged.AddDynamic(this, &ALogicGateSequence::OnPrerequisiteStateChanged);
		}
	}

	// Subscribe to ordered broadcasters
	for (AInteractableBroadcaster* Broadcaster : OrderedBroadcasters)
	{
		if (Broadcaster)
		{
			Broadcaster->OnInteractionBroadcast.AddDynamic(this, &ALogicGateSequence::OnBroadcasterStateChanged);
			UE_LOG(LogTemp, Log, TEXT("SequenceGate '%s': Subscribed to broadcaster '%s'"),
				*GetName(), *Broadcaster->GetName());
		}
	}

	// Check initial prerequisite state
	UpdateSequenceEnabled();
}

void ALogicGateSequence::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unsubscribe from prerequisites
	for (AActivableBase* Activable : RequiredActivables)
	{
		if (Activable)
		{
			Activable->OnActivationStateChanged.RemoveDynamic(this, &ALogicGateSequence::OnPrerequisiteStateChanged);
		}
	}

	// Unsubscribe from broadcasters
	for (AInteractableBroadcaster* Broadcaster : OrderedBroadcasters)
	{
		if (Broadcaster)
		{
			Broadcaster->OnInteractionBroadcast.RemoveDynamic(this, &ALogicGateSequence::OnBroadcasterStateChanged);
		}
	}

	Super::EndPlay(EndPlayReason);
}

bool ALogicGateSequence::ArePrerequisitesMet() const
{
	// If no prerequisites, always enabled
	if (RequiredActivables.Num() == 0)
	{
		return true;
	}

	// Check all prerequisites are active
	for (AActivableBase* Activable : RequiredActivables)
	{
		if (!Activable || !IActivable::Execute_IsActive(Cast<UObject>(Activable)))
		{
			return false;
		}
	}

	return true;
}

void ALogicGateSequence::OnPrerequisiteStateChanged(bool bIsActive, AActivableBase* Activable)
{
	UE_LOG(LogTemp, Log, TEXT("SequenceGate '%s': Prerequisite '%s' changed to %s"),
		*GetName(), *Activable->GetName(), bIsActive ? TEXT("ACTIVE") : TEXT("INACTIVE"));

	UpdateSequenceEnabled();
}

void ALogicGateSequence::UpdateSequenceEnabled()
{
	bool bNewEnabled = ArePrerequisitesMet();

	if (bSequenceEnabled != bNewEnabled)
	{
		bSequenceEnabled = bNewEnabled;

		UE_LOG(LogTemp, Warning, TEXT("SequenceGate '%s': Sequence %s"),
			*GetName(), bSequenceEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));

		BP_OnSequenceEnabledChanged(bSequenceEnabled);

		// Reset sequence when disabled
		if (!bSequenceEnabled)
		{
			ResetSequence();
		}
	}
}

void ALogicGateSequence::OnBroadcasterStateChanged(bool bIsActive, AInteractableBroadcaster* Broadcaster)
{
	// Only care about activation (button press)
	if (!bIsActive)
	{
		return;
	}

	// Ignore if sequence disabled or already complete
	if (!bSequenceEnabled || bSequenceComplete)
	{
		return;
	}

	// Check if this is the next expected broadcaster
	if (CurrentStep < OrderedBroadcasters.Num() && OrderedBroadcasters[CurrentStep] == Broadcaster)
	{
		// Correct! Advance sequence
		CurrentStep++;

		UE_LOG(LogTemp, Warning, TEXT("SequenceGate '%s': Correct step %d/%d - %s"),
			*GetName(), CurrentStep, OrderedBroadcasters.Num(), *Broadcaster->GetName());

		BP_OnSequenceProgress(CurrentStep, Broadcaster);

		// Check if sequence complete
		if (CurrentStep >= OrderedBroadcasters.Num())
		{
			bSequenceComplete = true;
			UE_LOG(LogTemp, Warning, TEXT("SequenceGate '%s': SEQUENCE COMPLETE!"), *GetName());

			BP_OnSequenceComplete();
			ActivateTargets();
		}
	}
	else
	{
		// Wrong button! Reset sequence
		UE_LOG(LogTemp, Warning, TEXT("SequenceGate '%s': WRONG button %s (expected step %d)"),
			*GetName(), *Broadcaster->GetName(), CurrentStep);

		BP_OnSequenceReset(Broadcaster);
		ResetSequence();
	}
}

void ALogicGateSequence::ResetSequence()
{
	CurrentStep = 0;
	bSequenceComplete = false;
}

void ALogicGateSequence::ActivateTargets()
{
	UE_LOG(LogTemp, Warning, TEXT("SequenceGate '%s': Activating %d targets"), *GetName(), TargetActivables.Num());

	for (AActivableBase* Target : TargetActivables)
	{
		if (Target && Target->Implements<UActivable>())
		{
			IActivable::Execute_SetActive(Cast<UObject>(Target), true);
		}
	}
}

void ALogicGateSequence::ManualReset()
{
	ResetSequence();
	UpdateSequenceEnabled();
}