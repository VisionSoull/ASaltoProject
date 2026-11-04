// Fill out your copyright notice in the Description page of Project Settings.

#include "CarryComponent.h"
#include "InteractionComponent.h"
#include "PushPullComponent.h"
#include "HandGripComponent.h"
#include "PushPullAnchorComponent.h"
#include "../Movables/MovableBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UCarryComponent::UCarryComponent()
{
	// Event driven: nothing runs per frame
	PrimaryComponentTick.bCanEverTick = false;
}

void UCarryComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	ensureMsgf(OwnerCharacter, TEXT("UCarryComponent must be on an ACharacter (owner: %s)"), *GetNameSafe(GetOwner()));
}

void UCarryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Drop();
	Super::EndPlay(EndPlayReason);
}

bool UCarryComponent::TryPickUp(AMovableBase* Object)
{
	if (!IsValid(OwnerCharacter) || !IsValid(Object) || !OwnerCharacter->GetMesh())
	{
		return false;
	}

	// One thing at a time: not while already carrying or pushing
	const UPushPullComponent* PushPull = OwnerCharacter->FindComponentByClass<UPushPullComponent>();
	if (CarriedObject || (PushPull && PushPull->IsHolding()))
	{
		FailPickUp(Object, EGrabFailReason::AlreadyHolding);
		return false;
	}

	if (!Object->IsInteractorAllowed(OwnerCharacter))
	{
		FailPickUp(Object, EGrabFailReason::NotAllowed);
		return false;
	}

	if (IMovable::Execute_IsLocked(Object))
	{
		FailPickUp(Object, EGrabFailReason::Locked);
		return false;
	}

	const EMovableWeight Weight = IMovable::Execute_GetWeight(Object);
	if (!MovableRules::IsCarryWeight(Weight))
	{
		FailPickUp(Object, EGrabFailReason::WeightNotSupported);
		return false;
	}

	if (Object->IsCarried())
	{
		FailPickUp(Object, EGrabFailReason::InUse);
		return false;
	}

	// Physics must be off before attaching
	Object->HandlePickedUp(this);
	CarriedObject = Object;

	USceneComponent* ObjectRoot = Object->GetRootComponent();
	if (MovableRules::IsOneHanded(Weight))
	{
		// Very Light: in the closer hand, at the object's hand offset
		USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
		const FVector ObjectLocation = Object->GetActorLocation();
		const bool bUseLeft = FVector::DistSquared(Mesh->GetSocketLocation(LeftHandBone), ObjectLocation)
			<= FVector::DistSquared(Mesh->GetSocketLocation(RightHandBone), ObjectLocation);

		ObjectRoot->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, bUseLeft ? LeftHandBone : RightHandBone);
		ObjectRoot->SetRelativeLocationAndRotation(Object->HandLocationOffset, Object->HandRotationOffset);
	}
	else
	{
		// Light: in front of the character, at the object's carry offset
		ObjectRoot->AttachToComponent(OwnerCharacter->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		ObjectRoot->SetRelativeLocationAndRotation(Object->CarryLocationOffset, Object->CarryRotationOffset);
		SetMovementSlowed(true, Object->CarrySpeedMultiplier);
	}

	OwnerCharacter->MoveIgnoreActorAdd(Object);
	SetFocusLocked(true);

	OnPickedUp.Broadcast(Object);
	return true;
}

void UCarryComponent::Drop()
{
	AMovableBase* Object = DetachCarried();
	if (!Object)
	{
		return;
	}

	Object->HandleDropped();

	// Put it down in front of the character (sweeps so it does not go through walls); physics lets it settle
	if (OwnerCharacter)
	{
		const FVector Front = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * DropDistance;
		const FVector DropLocation(Front.X, Front.Y, Object->GetActorLocation().Z);
		Object->SetActorLocation(DropLocation, true, nullptr, ETeleportType::TeleportPhysics);
	}

	OnDropped.Broadcast(Object);
}

void UCarryComponent::Throw()
{
	if (!IsValid(CarriedObject) || !MovableRules::IsThrowable(IMovable::Execute_GetWeight(CarriedObject)))
	{
		Drop();
		return;
	}

	const float ThrowSpeed = CarriedObject->ThrowSpeed;
	const float UpRadians = FMath::DegreesToRadians(CarriedObject->ThrowUpAngle);

	AMovableBase* Object = DetachCarried();
	if (!Object || !OwnerCharacter)
	{
		return;
	}

	// Character's facing, tilted up by the object's throw angle
	const FVector Forward = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	const FVector Direction = Forward * FMath::Cos(UpRadians) + FVector::UpVector * FMath::Sin(UpRadians);

	Object->HandleThrown(Direction * ThrowSpeed);
	OnThrown.Broadcast(Object);
}

void UCarryComponent::BeginRelease()
{
	if (CarriedObject && GetWorld())
	{
		ReleasePressTime = GetWorld()->GetTimeSeconds();
	}
}

void UCarryComponent::FinishRelease()
{
	// The release of the press that picked the object up has no BeginRelease: ignore it
	if (!CarriedObject || ReleasePressTime < 0.0 || !GetWorld())
	{
		return;
	}

	const double HeldSeconds = GetWorld()->GetTimeSeconds() - ReleasePressTime;
	ReleasePressTime = -1.0;

	if (HeldSeconds >= ThrowHoldSeconds && MovableRules::IsThrowable(IMovable::Execute_GetWeight(CarriedObject)))
	{
		Throw();
		return;
	}

	Drop();
}

float UCarryComponent::GetThrowHoldProgress() const
{
	if (!CarriedObject || ReleasePressTime < 0.0 || !GetWorld() || ThrowHoldSeconds <= 0.f)
	{
		return 0.f;
	}

	const float HeldSeconds = static_cast<float>(GetWorld()->GetTimeSeconds() - ReleasePressTime);
	return FMath::Clamp(HeldSeconds / ThrowHoldSeconds, 0.f, 1.f);
}

void UCarryComponent::FailPickUp(AMovableBase* Object, EGrabFailReason Reason)
{
	// Tell the object too, so its own Blueprint can react
	if (IsValid(Object))
	{
		Object->HandlePickUpFailed(OwnerCharacter, Reason);
	}
	OnPickUpFailed.Broadcast(Object, Reason);
}

AMovableBase* UCarryComponent::DetachCarried()
{
	AMovableBase* Object = CarriedObject;
	if (!Object)
	{
		return nullptr;
	}

	CarriedObject = nullptr;
	ReleasePressTime = -1.0;

	SetMovementSlowed(false, 1.f);
	SetFocusLocked(false);

	if (!IsValid(Object))
	{
		return nullptr;
	}

	Object->GetRootComponent()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	if (OwnerCharacter)
	{
		OwnerCharacter->MoveIgnoreActorRemove(Object);
	}
	return Object;
}

void UCarryComponent::SetMovementSlowed(bool bSlowed, float SpeedMultiplier)
{
	UCharacterMovementComponent* Movement = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return;
	}

	if (bSlowed && SavedMaxWalkSpeed < 0.f)
	{
		SavedMaxWalkSpeed = Movement->MaxWalkSpeed;
		Movement->MaxWalkSpeed *= SpeedMultiplier;
	}
	else if (!bSlowed && SavedMaxWalkSpeed >= 0.f)
	{
		Movement->MaxWalkSpeed = SavedMaxWalkSpeed;
		SavedMaxWalkSpeed = -1.f;
	}
}

void UCarryComponent::SetFocusLocked(bool bLocked)
{
	UInteractionComponent* Interaction = OwnerCharacter ? OwnerCharacter->FindComponentByClass<UInteractionComponent>() : nullptr;
	if (Interaction)
	{
		Interaction->SetFocusLocked(bLocked);
	}
}


void UCarryComponent::FindCarriedGrips(UHandGripComponent*& OutLeft, UHandGripComponent*& OutRight) const
{
	OutLeft = nullptr;
	OutRight = nullptr;

	// Very Light already sits in the hand bone: only two-handed (Light) objects drive hand IK
	if (!IsValid(CarriedObject) || !OwnerCharacter || MovableRules::IsOneHanded(IMovable::Execute_GetWeight(CarriedObject)))
	{
		return;
	}

	// Grips placed directly on the object come first
	CarriedObject->FindHandGrips(nullptr, OutLeft, OutRight);
	if (OutLeft || OutRight)
	{
		return;
	}

	// Otherwise reuse the push grips of the side facing the character
	UPushPullAnchorComponent* FacingAnchor = IMovable::Execute_GetFreeAnchorClosestTo(CarriedObject, OwnerCharacter->GetActorLocation());
	if (FacingAnchor)
	{
		CarriedObject->FindHandGrips(FacingAnchor, OutLeft, OutRight);
	}
}

void UCarryComponent::GetHandTargets(FVector& OutLeftLocation, FVector& OutRightLocation, bool& bOutHasLeft, bool& bOutHasRight) const
{
	UHandGripComponent* LeftGrip = nullptr;
	UHandGripComponent* RightGrip = nullptr;
	FindCarriedGrips(LeftGrip, RightGrip);

	UHandGripComponent::GetGripLocations(LeftGrip, RightGrip, OutLeftLocation, OutRightLocation, bOutHasLeft, bOutHasRight);
}

void UCarryComponent::GetHandTargetRotations(FRotator& OutLeftRotation, FRotator& OutRightRotation) const
{
	UHandGripComponent* LeftGrip = nullptr;
	UHandGripComponent* RightGrip = nullptr;
	FindCarriedGrips(LeftGrip, RightGrip);

	OutLeftRotation = LeftGrip ? LeftGrip->GetComponentRotation() : FRotator::ZeroRotator;
	OutRightRotation = RightGrip ? RightGrip->GetComponentRotation() : FRotator::ZeroRotator;
}