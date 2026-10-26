// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractionComponent.h"
#include "Components/SphereComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Character.h"
#include "../Interactables/Interactable.h"
#include "../Interactables/InteractableBase.h"
#include "DrawDebugHelpers.h"

// Builds the interaction sphere and turns Tick off, since detection runs on a timer instead.
UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetSphereRadius(InteractionRadius);
	InteractionZone->SetCollisionProfileName("InteractionZone");
	InteractionZone->SetComponentTickEnabled(false);
}

// Caches the owner. For the locally controlled character it attaches the sphere and hooks its overlaps.
void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());

	// Only run interaction detection for locally controlled character
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
	{
		return;
	}

	// InteractionRadius is only loaded after the constructor, so apply it here
	InteractionZone->SetSphereRadius(InteractionRadius);

	if (AActor* Owner = GetOwner())
	{
		InteractionZone->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		InteractionZone->RegisterComponent();
	}

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(GetOwner()->GetComponentByClass(UEnhancedInputComponent::StaticClass()));

	if (EnhancedInput && InteractAction)
	{
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &UInteractionComponent::DoInteractPressed);
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Completed, this, &UInteractionComponent::DoInteractReleased);
	}

	InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &UInteractionComponent::OnInteractionZoneBeginOverlap);
	InteractionZone->OnComponentEndOverlap.AddDynamic(this, &UInteractionComponent::OnInteractionZoneEndOverlap);

	// Anything already inside the sphere fired its BeginOverlap before the bindings above
	AddCurrentOverlaps();
}

// Clears the timer and drops focus on the current object before the component goes away.
void UInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InteractionCheckTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(InteractionCheckTimer);
	}

	if (CurrentInteractable.GetInterface())
	{
		IInteractable::Execute_EndFocus(CurrentInteractable.GetObject(), OwnerCharacter, CurrentHitComponent);
	}

	for (const TScriptInterface<IInteractable>& Interactable : NearbyInteractables)
	{
		if (IsValid(Interactable.GetObject()))
		{
			IInteractable::Execute_EndRange(Interactable.GetObject(), OwnerCharacter);
		}
	}
	NearbyInteractables.Empty();

	Super::EndPlay(EndPlayReason);
}

// A candidate entered the sphere.
void UInteractionComponent::OnInteractionZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AddCandidate(OtherActor);
}

// Stores an interactable in range and starts the check timer if it is the first one.
void UInteractionComponent::AddCandidate(AActor* Actor)
{
	if (!IsValid(Actor) || Actor == GetOwner() || !Actor->Implements<UInteractable>())
	{
		return;
	}

	// An actor with several overlapping primitives must only enter range once
	const bool bAlreadyInRange = NearbyInteractables.ContainsByPredicate(
		[Actor](const TScriptInterface<IInteractable>& Existing) { return Existing.GetObject() == Actor; });
	if (bAlreadyInRange)
	{
		return;
	}

	TScriptInterface<IInteractable> Interactable;
	Interactable.SetObject(Actor);
	Interactable.SetInterface(Cast<IInteractable>(Actor));

	NearbyInteractables.Add(Interactable);
	IInteractable::Execute_BeginRange(Actor, OwnerCharacter);

	if (NearbyInteractables.Num() == 1)
	{
		CurrentCheckRate = InteractionCheckInterval;
		GetWorld()->GetTimerManager().SetTimer(InteractionCheckTimer, this, &UInteractionComponent::CheckAimedInteractable,
			InteractionCheckInterval, true);
	}
}

// Overlap events only fire on entering, so read what the sphere already overlaps.
void UInteractionComponent::AddCurrentOverlaps()
{
	InteractionZone->UpdateOverlaps();

	TArray<AActor*> OverlappingActors;
	InteractionZone->GetOverlappingActors(OverlappingActors);
	for (AActor* Actor : OverlappingActors)
	{
		AddCandidate(Actor);
	}
}

// Resizes the interaction sphere at runtime.
void UInteractionComponent::SetInteractionRadius(float NewRadius)
{
	InteractionRadius = FMath::Max(NewRadius, 1.0f);

	// Before BeginPlay, or on a character that is not locally controlled, the sphere is not set up yet; BeginPlay applies the radius
	if (!InteractionZone->IsRegistered())
	{
		return;
	}

	// Updating overlaps sends BeginOverlap / EndOverlap for objects that cross the new edge, so range enter / exit fire as usual
	InteractionZone->SetSphereRadius(InteractionRadius, true);
}

// A candidate left the sphere: drop it, clear focus if it was the focused one, and stop the timer when none are left.
void UInteractionComponent::OnInteractionZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor || OtherActor == GetOwner())
	{
		return;
	}

	const int32 RemovedCount = NearbyInteractables.RemoveAll([OtherActor](const TScriptInterface<IInteractable>& Interactable)
		{
			return Interactable.GetObject() == OtherActor;
		});

	if (RemovedCount > 0)
	{
		IInteractable::Execute_EndRange(OtherActor, OwnerCharacter);
	}

	if (CurrentInteractable.GetObject() == OtherActor)
	{
		UpdateCurrentInteractable(TScriptInterface<IInteractable>(), nullptr);
	}

	if (NearbyInteractables.Num() == 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(InteractionCheckTimer);
	}
}

// Timer body: score every candidate, keep the best one that has line of sight, and update the focus.
void UInteractionComponent::CheckAimedInteractable()
{
	// Holding something: keep focus on it so EndInteract reaches the right object
	if (bFocusLocked)
	{
		return;
	}

	if (NearbyInteractables.Num() == 0)
	{
		UpdateCurrentInteractable(TScriptInterface<IInteractable>(), nullptr);
		ApplyCheckInterval(false);
		return;
	}

	if (!OwnerCharacter)
	{
		return;
	}

	const FVector Origin = GetDetectionOrigin();
	const FVector Facing = GetFacingDirection();
	const float DebugLife = (CurrentCheckRate > 0.0f ? CurrentCheckRate : InteractionCheckInterval) * 1.5f;

	// Score every nearby candidate by facing + distance, keep the highest
	TScriptInterface<IInteractable> BestInteractable;
	UPrimitiveComponent* BestComponent = nullptr;
	AActor* BestActor = nullptr;
	float BestScore = MinScoreThreshold;

	for (const TScriptInterface<IInteractable>& Candidate : NearbyInteractables)
	{
		AActor* CandidateActor = Cast<AActor>(Candidate.GetObject());
		if (!CandidateActor)
		{
			continue;
		}

		// Role gate: objects this character is not allowed to use never get focus
		const AInteractableBase* CandidateBase = Cast<AInteractableBase>(CandidateActor);
		if (CandidateBase && !CandidateBase->IsInteractorAllowed(OwnerCharacter))
		{
			if (bDebugDraw)
			{
				DrawDebugLine(GetWorld(), Origin, CandidateActor->GetActorLocation(), FColor(80, 80, 80), false, DebugLife, 0, 1.0f);
			}
			continue;
		}

		const FVector ToCandidate = CandidateActor->GetActorLocation() - Origin;
		const float Distance = ToCandidate.Size();
		const float FacingDot = FVector::DotProduct(Facing, ToCandidate.GetSafeNormal());

		// Cone gate: ignore anything outside the facing cone
		if (FacingDot < MinFacingDot)
		{
			if (bDebugDraw)
			{
				DrawDebugLine(GetWorld(), Origin, CandidateActor->GetActorLocation(), FColor(80, 80, 80), false, DebugLife, 0, 1.0f);
			}
			continue;
		}

		const float Score = ScoreInteractable(CandidateActor, Distance, FacingDot);

		if (bDebugDraw)
		{
			DrawDebugLine(GetWorld(), Origin, CandidateActor->GetActorLocation(), FColor::Yellow, false, DebugLife, 0, 1.5f);
			DrawDebugString(GetWorld(), CandidateActor->GetActorLocation() + FVector(0, 0, 20),
				FString::Printf(TEXT("%.2f"), Score), nullptr, FColor::White, DebugLife);
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			BestInteractable = Candidate;
			BestActor = CandidateActor;
			BestComponent = ResolveFocusComponent(CandidateActor, nullptr);
		}
	}

	// Occlusion: the chosen target must have line of sight, otherwise drop it
	if (bRequireLineOfSight && BestActor && !HasLineOfSightTo(BestActor))
	{
		if (bDebugDraw)
		{
			DrawDebugLine(GetWorld(), Origin, BestActor->GetActorLocation(), FColor::Red, false, DebugLife, 0, 2.0f);
		}
		BestInteractable = TScriptInterface<IInteractable>();
		BestComponent = nullptr;
		BestActor = nullptr;
	}

	if (BestInteractable.GetObject() != CurrentInteractable.GetObject() || BestComponent != CurrentHitComponent)
	{
		UpdateCurrentInteractable(BestInteractable, BestComponent);
	}

	if (bDebugDraw)
	{
		DrawDebugSphere(GetWorld(), OwnerCharacter->GetActorLocation(), InteractionRadius, 16, FColor(0, 128, 255), false, DebugLife, 0, 1.0f);
		DrawDebugLine(GetWorld(), Origin, Origin + Facing * TraceDistance, FColor::Cyan, false, DebugLife, 0, 2.0f);
		if (BestActor)
		{
			DrawDebugLine(GetWorld(), Origin, BestActor->GetActorLocation(), FColor::Green, false, DebugLife, 0, 3.0f);
			DrawDebugSphere(GetWorld(), BestActor->GetActorLocation(), 30.0f, 12, FColor::Green, false, DebugLife, 0, 2.0f);
		}
	}

	// Adaptive rate: poll faster while something is focused, slower when only nearby
	ApplyCheckInterval(CurrentInteractable.GetObject() != nullptr);
}

// World point traces and facing start from: character chest height, else the trace origin, else the owner.
FVector UInteractionComponent::GetDetectionOrigin() const
{
	if (bTraceFromCharacter && OwnerCharacter)
	{
		return OwnerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, TraceHeight);
	}

	if (TraceOrigin)
	{
		return TraceOrigin->GetComponentLocation();
	}

	return GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
}

// Facing used for scoring: the custom direction if set, else the character or trace-origin forward.
FVector UInteractionComponent::GetFacingDirection() const
{
	if (bUseCustomDirection)
	{
		return CustomTraceDirection.GetSafeNormal();
	}

	if (bTraceFromCharacter && OwnerCharacter)
	{
		return OwnerCharacter->GetActorForwardVector();
	}

	if (TraceOrigin)
	{
		return TraceOrigin->GetForwardVector();
	}

	return FVector::ForwardVector;
}

// Which primitive gets the outline: the aimed one if there is one, else the actor's first primitive.
UPrimitiveComponent* UInteractionComponent::ResolveFocusComponent(AActor* Actor, UPrimitiveComponent* AimedComponent) const
{
	if (AimedComponent)
	{
		return AimedComponent;
	}

	return Actor ? Actor->FindComponentByClass<UPrimitiveComponent>() : nullptr;
}

// True when nothing on the Visibility channel blocks the straight line from the origin to the target.
bool UInteractionComponent::HasLineOfSightTo(AActor* Target) const
{
	if (!Target || !GetWorld())
	{
		return false;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(Target);

	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		GetDetectionOrigin(),
		Target->GetActorLocation(),
		ECC_Visibility,
		QueryParams
	);

	return !bBlocked;
}

// Switches the timer between the idle rate and the faster focused rate, and only when it actually changes.
void UInteractionComponent::ApplyCheckInterval(bool bFocused)
{
	if (!InteractionCheckTimer.IsValid() || !GetWorld())
	{
		return;
	}

	const float Desired = bFocused ? FocusedCheckInterval : InteractionCheckInterval;
	if (FMath::IsNearlyEqual(CurrentCheckRate, Desired))
	{
		return;
	}

	CurrentCheckRate = Desired;
	GetWorld()->GetTimerManager().SetTimer(InteractionCheckTimer, this, &UInteractionComponent::CheckAimedInteractable, Desired, true);
}

// Default score: facing and distance each remapped to 0..1, then weighted. Override in Blueprint to change it.
float UInteractionComponent::ScoreInteractable_Implementation(AActor* Interactable, float Distance, float FacingDot) const
{
	// Facing: remap [MinFacingDot..1] -> [0..1] so the weight is comparable to distance
	const float FacingRange = FMath::Max(1.0f - MinFacingDot, KINDA_SMALL_NUMBER);
	const float Facing01 = FMath::Clamp((FacingDot - MinFacingDot) / FacingRange, 0.0f, 1.0f);

	// Distance: closer -> 1
	const float Distance01 = FMath::Clamp(1.0f - (Distance / FMath::Max(InteractionRadius, 1.0f)), 0.0f, 1.0f);

	return (FacingWeight * Facing01) + (DistanceWeight * Distance01);
}

// Swap focus: EndFocus on the old object, store the new one, then BeginFocus on it.
void UInteractionComponent::UpdateCurrentInteractable(TScriptInterface<IInteractable> NewInteractable, UPrimitiveComponent* HitComponent)
{
	if (CurrentInteractable.GetInterface() && CurrentHitComponent)
	{
		IInteractable::Execute_EndFocus(CurrentInteractable.GetObject(), OwnerCharacter, CurrentHitComponent);
	}

	CurrentInteractable = NewInteractable;
	CurrentHitComponent = HitComponent;

	if (CurrentInteractable.GetInterface() && CurrentHitComponent)
	{
		IInteractable::Execute_BeginFocus(CurrentInteractable.GetObject(), OwnerCharacter, CurrentHitComponent);
	}
}

// Interact pressed: on the server call BeginInteract directly, on a client send it through the server RPC.
void UInteractionComponent::DoInteractPressed()
{
	if (!CurrentInteractable.GetInterface())
	{
		return;
	}

	AInteractableBase* Interactable = Cast<AInteractableBase>(CurrentInteractable.GetObject());
	if (!Interactable)
	{
		return;
	}

	IInteractable::Execute_BeginInteract(Interactable, OwnerCharacter);
}

// Interact released: same server or client split as press, but calls EndInteract, which Hold interactions need.
void UInteractionComponent::DoInteractReleased()
{
	if (!CurrentInteractable.GetInterface())
	{
		return;
	}

	AInteractableBase* Interactable = Cast<AInteractableBase>(CurrentInteractable.GetObject());
	if (!Interactable)
	{
		return;
	}

	IInteractable::Execute_EndInteract(Interactable, OwnerCharacter);
}

TArray<AActor*> UInteractionComponent::GetNearbyInteractableActors() const
{
	TArray<AActor*> Result;
	for (const TScriptInterface<IInteractable>& Interactable : NearbyInteractables)
	{
		if (AActor* Actor = Cast<AActor>(Interactable.GetObject()))
		{
			Result.Add(Actor);
		}
	}
	return Result;
}