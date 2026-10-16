// Fill out your copyright notice in the Description page of Project Settings.

#include "PushPullComponent.h"
#include "InteractionComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../Movables/Movable.h"
#include "PushPullAnchorComponent.h"
#include "../Interactables/InteractableBase.h"
#include "../Settings/InteractionSettings.h"

UPushPullComponent::UPushPullComponent()
{
	// Ticks only while holding something
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UPushPullComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	ensureMsgf(OwnerCharacter, TEXT("UPushPullComponent must be on an ACharacter (owner: %s)"), *GetNameSafe(GetOwner()));
}

void UPushPullComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Release();
	Super::EndPlay(EndPlayReason);
}

bool UPushPullComponent::TryGrab(AActor* Object)
{
	if (!OwnerCharacter || !Object || !Object->Implements<UMovable>())
	{
		return false;
	}

	if (HeldObject)
	{
		FailGrab(Object, EGrabFailReason::AlreadyHolding);
		return false;
	}

	const AInteractableBase* Interactable = Cast<AInteractableBase>(Object);
	if (Interactable && !Interactable->IsInteractorAllowed(OwnerCharacter))
	{
		FailGrab(Object, EGrabFailReason::NotAllowed);
		return false;
	}

	if (IMovable::Execute_IsLocked(Object))
	{
		FailGrab(Object, EGrabFailReason::Locked);
		return false;
	}

	if (!MovableRules::IsWeightSupported(IMovable::Execute_GetWeight(Object)))
	{
		UE_LOG(LogTemp, Warning, TEXT("PushPull: %s cannot be grabbed yet, only Heavy and Very Heavy objects are supported"), *GetNameSafe(Object));
		FailGrab(Object, EGrabFailReason::WeightNotSupported);
		return false;
	}

	UPushPullAnchorComponent* Anchor = IMovable::Execute_GetFreeAnchorClosestTo(Object, OwnerCharacter->GetActorLocation());
	if (!Anchor || !IMovable::Execute_RegisterMover(Object, this, Anchor))
	{
		FailGrab(Object, EGrabFailReason::NoFreeAnchor);
		return false;
	}

	HeldObject = Object;
	HeldAnchor = Anchor;
	bIsAligned = false;
	PushAmount = 0.f;
	AimElapsed = 0.f;
	DragInput = FVector::ZeroVector;
	AimStartLocation = OwnerCharacter->GetActorLocation();
	AimStartRotation = OwnerCharacter->GetActorRotation();

	LastLocation = AimStartLocation;
	MoveSpeed = 0.f;

	LockMovement(true);
	LockFocus(true);

	// The capsule follows the object at a fixed gap, so it must not bump into it
	OwnerCharacter->MoveIgnoreActorAdd(Object);
	SetComponentTickEnabled(true);

	OnGrabbed.Broadcast(Object);
	return true;
}

void UPushPullComponent::Release()
{
	if (!HeldObject)
	{
		return;
	}

	AActor* Object = HeldObject;
	HeldObject = nullptr;
	HeldAnchor = nullptr;
	bIsAligned = false;
	PushAmount = 0.f;
	DragInput = FVector::ZeroVector;
	MoveSpeed = 0.f;

	if (IsValid(Object))
	{
		IMovable::Execute_UnregisterMover(Object, this);
		if (OwnerCharacter)
		{
			OwnerCharacter->MoveIgnoreActorRemove(Object);
		}
	}

	LockMovement(false);
	LockFocus(false);
	SetComponentTickEnabled(false);

	OnReleased.Broadcast(Object);
}

void UPushPullComponent::SetMoveInput(FVector Input)
{
	ExternalInput = Input;
	bHasExternalInput = true;
}

void UPushPullComponent::ClearMoveInput()
{
	ExternalInput = FVector::ZeroVector;
	bHasExternalInput = false;
}

FVector UPushPullComponent::GetMoveInput() const
{
	if (bHasExternalInput)
	{
		return ExternalInput;
	}
	// Still recorded while MaxWalkSpeed is 0, so the player's stick/keys drive the object
	return OwnerCharacter ? OwnerCharacter->GetLastMovementInputVector() : FVector::ZeroVector;
}

void UPushPullComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!HeldObject)
	{
		SetComponentTickEnabled(false);
		return;
	}

	// Object destroyed or locked mid-drag
	if (!OwnerCharacter || !IsValid(HeldObject) || !HeldAnchor || IMovable::Execute_IsLocked(HeldObject))
	{
		Release();
		return;
	}

	// Speed the character moved at since last frame, for the Anim BP (CharacterMovement's velocity stays 0)
	const FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	MoveSpeed = DeltaTime > 0.f ? FVector::Dist2D(CurrentLocation, LastLocation) / DeltaTime : 0.f;
	LastLocation = CurrentLocation;

	const FVector AnchorLocation = HeldAnchor->GetComponentLocation();
	const FVector StandLocation(AnchorLocation.X, AnchorLocation.Y, OwnerCharacter->GetActorLocation().Z);
	const FRotator Facing(0.f, HeldAnchor->GetComponentRotation().Yaw, 0.f);

	if (!bIsAligned)
	{
		// Slide into the grab spot, facing the object
		AimElapsed += DeltaTime;
		const float Alpha = AimingDuration > 0.f ? FMath::Clamp(AimElapsed / AimingDuration, 0.f, 1.f) : 1.f;
		// Ease in and out so the slide starts and stops smoothly
		const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
		OwnerCharacter->SetActorLocation(FMath::Lerp(AimStartLocation, StandLocation, Eased), true);
		OwnerCharacter->SetActorRotation(FMath::Lerp(AimStartRotation, Facing, Eased));
		bIsAligned = Alpha >= 1.f;
		if (bIsAligned)
		{
			OnReadyToPush.Broadcast(HeldObject);
		}
		return;
	}

	const UInteractionSettings* Settings = UInteractionSettings::GetInteractionSettings();

	// The grab spot moved away (object stuck or character blocked): let go
	if (FVector::Dist2D(OwnerCharacter->GetActorLocation(), StandLocation) > Settings->PushPullBreakDistance)
	{
		Release();
		return;
	}

	// Stay glued to the grab spot as the object moves
	OwnerCharacter->SetActorLocation(StandLocation, true);
	OwnerCharacter->SetActorRotation(Facing);

	// How much the input points into the object (+) or away from it (-)
	const FVector Input = GetMoveInput();
	const float Along = FMath::Clamp(FVector::DotProduct(FVector(Input.X, Input.Y, 0.f), Facing.Vector()), -1.f, 1.f);
	PushAmount = FMath::Abs(Along) < Settings->PushPullInputDeadZone ? 0.f : Along;

	// Drop the direction the object does not accept (push only / pull only)
	PushAmount = MovableRules::FilterPushAmount(PushAmount, IMovable::Execute_GetPushPullMode(HeldObject));

	// Full ground input, for objects that move freely
	const FVector FlatInput(Input.X, Input.Y, 0.f);
	DragInput = FlatInput.Size() < Settings->PushPullInputDeadZone ? FVector::ZeroVector : FlatInput.GetClampedToMaxSize(1.f);
}

void UPushPullComponent::FailGrab(AActor* Object, EGrabFailReason Reason)
{
	OnGrabFailed.Broadcast(Object, Reason);
}

void UPushPullComponent::LockMovement(bool bLock)
{
	UCharacterMovementComponent* Movement = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return;
	}

	if (bLock && SavedMaxWalkSpeed < 0.f)
	{
		SavedMaxWalkSpeed = Movement->MaxWalkSpeed;
		bSavedOrientRotationToMovement = Movement->bOrientRotationToMovement;
		Movement->MaxWalkSpeed = 0.f;
		Movement->bOrientRotationToMovement = false;
	}
	else if (!bLock && SavedMaxWalkSpeed >= 0.f)
	{
		Movement->MaxWalkSpeed = SavedMaxWalkSpeed;
		Movement->bOrientRotationToMovement = bSavedOrientRotationToMovement;
		SavedMaxWalkSpeed = -1.f;
	}
}

void UPushPullComponent::LockFocus(bool bLock)
{
	UInteractionComponent* Interaction = OwnerCharacter ? OwnerCharacter->FindComponentByClass<UInteractionComponent>() : nullptr;
	if (Interaction)
	{
		Interaction->SetFocusLocked(bLock);
	}
}