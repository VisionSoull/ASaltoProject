#include "StealthComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Touch.h"
#include "GameFramework/Actor.h"

UStealthComponent::UStealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	StimuliSource = nullptr;
}

void UStealthComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Reuse an existing stimuli source on the owner if one's already there, otherwise create one.
	StimuliSource = Owner->FindComponentByClass<UAIPerceptionStimuliSourceComponent>();
	if (!StimuliSource)
	{
		StimuliSource = NewObject<UAIPerceptionStimuliSourceComponent>(Owner, TEXT("StealthStimuliSource"));
		StimuliSource->RegisterComponent();
	}

	// Register for whichever senses are enabled, then push the registration to the perception
	// system ONCE at the end - RegisterWithPerceptionSystem() applies to every sense registered
	// so far, calling it once per sense is redundant. A stimuli source that's registered for a
	// sense but never pushed with RegisterWithPerceptionSystem() silently senses nothing.
	
	if (bRegisterSight)
	{
		StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
	}

	if (bRegisterHearing)
	{
		StimuliSource->RegisterForSense(UAISense_Hearing::StaticClass());
	}

	if (bRegisterDamage)
	{
		StimuliSource->RegisterForSense(UAISense_Damage::StaticClass());
	}

	if (bRegisterTouch)
	{
		StimuliSource->RegisterForSense(UAISense_Touch::StaticClass());
	}

	StimuliSource->RegisterWithPerceptionSystem();
}

void UStealthComponent::ReportDetection(AActor* Instigator, FGameplayTag Reason, FVector Location)
{
	if (!bDetectionEnabled)
	{
		if (bDrawDebug)
		{
			UE_LOG(LogTemp, Log, TEXT("StealthComponent: ReportDetection ignored (disabled) on %s"),
				*GetNameSafe(GetOwner()));
		}
		return;
	}

	if (CurrentState == EStealthState::Detected)
	{
		if (bDrawDebug)
		{
			UE_LOG(LogTemp, Log, TEXT("StealthComponent: ReportDetection ignored (already latched) on %s"),
				*GetNameSafe(GetOwner()));
		}
		return;
	}

	CurrentState = EStealthState::Detected;

	LastDetectionEvent.Instigator = Instigator;
	LastDetectionEvent.Reason = Reason;
	LastDetectionEvent.Location = Location;

	if (bDrawDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("StealthComponent: Player detected on %s by %s, reason %s"),
			*GetNameSafe(GetOwner()), *GetNameSafe(Instigator), *Reason.ToString());
	}

	OnPlayerDetected.Broadcast(LastDetectionEvent);
}

void UStealthComponent::ResetDetection()
{
	CurrentState = EStealthState::Hidden;

	if (bDrawDebug)
	{
		UE_LOG(LogTemp, Log, TEXT("StealthComponent: Reset to Hidden on %s"), *GetNameSafe(GetOwner()));
	}
}

void UStealthComponent::SetDetectionEnabled(bool bEnabled)
{
	bDetectionEnabled = bEnabled;
}
