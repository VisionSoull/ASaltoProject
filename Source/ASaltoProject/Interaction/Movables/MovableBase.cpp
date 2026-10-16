// Fill out your copyright notice in the Description page of Project Settings.

#include "MovableBase.h"
#include "../Components/PushPullAnchorComponent.h"
#include "../Components/PushPullComponent.h"
#include "../Components/InteractionFeedbackComponent.h"
#include "../Settings/InteractionSettings.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"

AMovableBase::AMovableBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("Root")))
{
	// A physics body must be the root, so the actor (and the interaction focus) moves with it
	SetRootComponent(BaseMesh);
	FeedbackComponent->SetupAttachment(BaseMesh);

	// Solid and still until dragged; lock zones need overlap events from it
	BaseMesh->SetSimulatePhysics(false);
	BaseMesh->SetGenerateOverlapEvents(true);

	// Movables collide with the world and characters (the Interactable preset ignores them)
	BaseMesh->SetCollisionProfileName(TEXT("Movable"));

	DragHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("DragHandle"));

	// Ticks only while someone holds it
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AMovableBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseAllMovers();
	Super::EndPlay(EndPlayReason);
}

// ---------------- Lock ----------------

void AMovableBase::Lock()
{
	if (bLocked)
	{
		return;
	}
	bLocked = true;

	ReleaseAllMovers();
	StopDrag();
	ReceiveLocked();
	OnLocked.Broadcast(this);
}

void AMovableBase::Unlock()
{
	if (!bLocked)
	{
		return;
	}
	bLocked = false;
	ReceiveUnlocked();
	OnUnlocked.Broadcast(this);
}

void AMovableBase::ReleaseAllMovers()
{
	// Release() calls UnregisterMover, which edits Movers, so loop over a copy
	const TArray<FMovableMoverSlot> Snapshot = Movers;
	for (const FMovableMoverSlot& Slot : Snapshot)
	{
		if (Slot.Mover)
		{
			Slot.Mover->Release();
		}
	}
}

// ---------------- Interaction ----------------

void AMovableBase::BeginInteract_Implementation(ACharacter* Character)
{
	UPushPullComponent* PushPull = Character ? Character->FindComponentByClass<UPushPullComponent>() : nullptr;
	if (!PushPull)
	{
		return;
	}

	// Toggle: pressing again on the object we hold lets go
	if (GrabMode == EGrabMode::Toggle && PushPull->GetHeldObject() == this)
	{
		PushPull->Release();
		return;
	}

	PushPull->TryGrab(this);
}

void AMovableBase::EndInteract_Implementation(ACharacter* Character)
{
	if (GrabMode != EGrabMode::Hold)
	{
		return;
	}

	if (UPushPullComponent* PushPull = FindMoverOf(Character))
	{
		PushPull->Release();
	}
}

// ---------------- IMovable ----------------

bool AMovableBase::IsLocked_Implementation() const
{
	return bLocked;
}

EMovableWeight AMovableBase::GetWeight_Implementation() const
{
	return Weight;
}

EGrabMode AMovableBase::GetGrabMode_Implementation() const
{
	return GrabMode;
}

EPushPullMode AMovableBase::GetPushPullMode_Implementation() const
{
	return PushPullMode;
}

UPushPullAnchorComponent* AMovableBase::GetFreeAnchorClosestTo_Implementation(const FVector& FromLocation) const
{
	TInlineComponentArray<UPushPullAnchorComponent*> Anchors(this);

	UPushPullAnchorComponent* Closest = nullptr;
	float ClosestDistSq = TNumericLimits<float>::Max();
	for (UPushPullAnchorComponent* Anchor : Anchors)
	{
		if (IsAnchorTaken(Anchor))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Anchor->GetComponentLocation(), FromLocation);
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			Closest = Anchor;
		}
	}
	return Closest;
}

bool AMovableBase::RegisterMover_Implementation(UPushPullComponent* Mover, UPushPullAnchorComponent* Anchor)
{
	if (!Mover || !Anchor || bLocked || !MovableRules::IsWeightSupported(Weight) || IsMover(Mover) || IsAnchorTaken(Anchor))
	{
		return false;
	}

	FMovableMoverSlot Slot;
	Slot.Mover = Mover;
	Slot.Anchor = Anchor;
	Movers.Add(Slot);

	SetActorTickEnabled(true);
	OnMoverGrabbed(Cast<ACharacter>(Mover->GetOwner()));
	return true;
}

void AMovableBase::UnregisterMover_Implementation(UPushPullComponent* Mover)
{
	const int32 Index = Movers.IndexOfByPredicate([Mover](const FMovableMoverSlot& Slot) { return Slot.Mover == Mover; });
	if (Index == INDEX_NONE)
	{
		return;
	}
	ACharacter* Character = Mover ? Cast<ACharacter>(Mover->GetOwner()) : nullptr;
	Movers.RemoveAt(Index);

	if (Movers.Num() == 0)
	{
		StopDrag();
		SetActorTickEnabled(false);
	}

	OnMoverReleased(Character);
}

// ---------------- Drag ----------------

void AMovableBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Add up what every mover sitting at its grab spot asks for
	FVector FirstAxis = FVector::ZeroVector;
	FVector Combined = FVector::ZeroVector;
	int32 ReadyMovers = 0;
	bool bAllPushing = true;
	TArray<ACharacter*, TInlineAllocator<2>> NewlyReady;
	for (FMovableMoverSlot& Slot : Movers)
	{
		if (!Slot.Mover || !Slot.Anchor || !Slot.Mover->IsAligned())
		{
			continue;
		}

		if (!Slot.bReady)
		{
			Slot.bReady = true;
			NewlyReady.Add(Cast<ACharacter>(Slot.Mover->GetOwner()));
		}

		const FVector Axis = Slot.Anchor->GetForwardVector().GetSafeNormal2D();
		if (FirstAxis.IsNearlyZero())
		{
			FirstAxis = Axis;
		}

		const FVector Contribution = MoveConstraint == EMoveConstraint::Free
			? Slot.Mover->GetDragInput()
			: Axis * Slot.Mover->GetPushAmount();

		Combined += Contribution;
		bAllPushing &= !Contribution.IsNearlyZero();
		++ReadyMovers;
	}

	// Fired after the loop: the Blueprint may release a mover here, which edits Movers
	for (ACharacter* Character : NewlyReady)
	{
		OnMoverReady(Character);
	}
	if (Movers.Num() == 0)
	{
		return;
	}

	// Single Axis: keep only the axis of the first grab spot
	if (MoveConstraint == EMoveConstraint::SingleAxis)
	{
		Combined = MovableRules::ProjectOnAxis(Combined, FirstAxis);
	}
	const FVector Direction = Combined.GetSafeNormal2D();

	// Very Heavy needs two characters pushing at once; if one stops, the object stops
	const bool bEnoughMovers = ReadyMovers >= MovableRules::GetRequiredMovers(Weight);
	if (!bEnoughMovers || !bAllPushing || Direction.IsNearlyZero())
	{
		StopDrag();
		return;
	}

	StartDrag();

	FVector Target = BaseMesh->GetComponentLocation() + Direction * DragSpeed * DeltaTime;

	// Stay on the floor it started on
	Target.Z = DragGroundZ;
	DragHandle->SetTargetLocation(Target);
}

void AMovableBase::StartDrag()
{
	if (bDragging)
	{
		return;
	}
	bDragging = true;

	DragGroundZ = BaseMesh->GetComponentLocation().Z;

	// Physics only while dragged; the mover's capsule follows it, so it ignores pawns meanwhile
	BaseMesh->SetSimulatePhysics(true);
	BaseMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	DragHandle->GrabComponentAtLocationWithRotation(BaseMesh, NAME_None, BaseMesh->GetComponentLocation(), BaseMesh->GetComponentRotation());

	OnDragStarted();
}

void AMovableBase::StopDrag()
{
	if (!bDragging)
	{
		return;
	}
	bDragging = false;

	DragHandle->ReleaseComponent();
	BaseMesh->SetSimulatePhysics(false);
	BaseMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	OnDragStopped();
}

// ---------------- Helpers ----------------

UPushPullComponent* AMovableBase::FindMoverOf(const ACharacter* Character) const
{
	for (const FMovableMoverSlot& Slot : Movers)
	{
		if (Slot.Mover && Slot.Mover->GetOwner() == Character)
		{
			return Slot.Mover;
		}
	}
	return nullptr;
}

bool AMovableBase::IsMover(const UPushPullComponent* Mover) const
{
	return Movers.ContainsByPredicate([Mover](const FMovableMoverSlot& Slot) { return Slot.Mover == Mover; });
}

bool AMovableBase::IsAnchorTaken(const UPushPullAnchorComponent* Anchor) const
{
	return Movers.ContainsByPredicate([Anchor](const FMovableMoverSlot& Slot) { return Slot.Anchor == Anchor; });
}