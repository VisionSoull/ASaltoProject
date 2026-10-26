// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EMovableWeightType.h"
#include "EMovableTypes.generated.h"

/** How a movable is held. */
UENUM(BlueprintType)
enum class EGrabMode : uint8
{
	/** Held while the interact button is held down */
	Hold,
	/** First press grabs, second press lets go */
	Toggle
};

/** Why a grab was refused. */
UENUM(BlueprintType)
enum class EGrabFailReason : uint8
{
	/** The object's Allowed Interactor refuses this character */
	NotAllowed,
	/** The object is locked */
	Locked,
	/** Every grab spot is already taken */
	NoFreeAnchor,
	/** This character already holds another object */
	AlreadyHolding,
	/** Only Heavy and Very Heavy objects can be grabbed for now */
	WeightNotSupported,
	/** The free grab spot is on another face of the object; walk to that face first */
	WrongSide,
	/** Someone else is already carrying this object */
	InUse
};

/** Which directions a movable accepts. */
UENUM(BlueprintType)
enum class EPushPullMode : uint8
{
	/** Can only be pushed (moved away from the character) */
	PushOnly	UMETA(DisplayName = "Push Only"),
	/** Can only be pulled (moved towards the character) */
	PullOnly	UMETA(DisplayName = "Pull Only"),
	/** Can be pushed and pulled */
	PushAndPull	UMETA(DisplayName = "Push and Pull")
};

/** How a movable moves while dragged. */
UENUM(BlueprintType)
enum class EMoveConstraint : uint8
{
	/** Only along the axis of the grabbed side (push / pull) */
	SingleAxis	UMETA(DisplayName = "Single Axis"),
	/** In any direction on the ground, following the movement input */
	Free
};

/** Which hand a grip point is for. */
UENUM(BlueprintType)
enum class EHandSide : uint8
{
	Left,
	Right
};

namespace MovableRules
{
	/** Weights that are pushed and pulled. */
	inline bool IsWeightSupported(EMovableWeight Weight)
	{
		return Weight == EMovableWeight::Heavy || Weight == EMovableWeight::VeryHeavy;
	}

	/** How many characters must push at the same time for the object to move. */
	inline int32 GetRequiredMovers(EMovableWeight Weight)
	{
		return Weight == EMovableWeight::VeryHeavy ? 2 : 1;
	}

	/** Very Light and Light are carried in the hands instead of pushed. */
	inline bool IsCarryWeight(EMovableWeight Weight)
	{
		return Weight == EMovableWeight::VeryLight || Weight == EMovableWeight::Light;
	}

	/** Only Very Light objects can be thrown; Light ones are only picked up and dropped. */
	inline bool IsThrowable(EMovableWeight Weight)
	{
		return Weight == EMovableWeight::VeryLight;
	}

	/** Very Light is held in one hand; Light needs both. */
	inline bool IsOneHanded(EMovableWeight Weight)
	{
		return Weight == EMovableWeight::VeryLight;
	}

	/** Grab spots on the same face push in (almost) the same direction. */
	constexpr float SameFaceMinDot = 0.95f;

	/** How far around the face a character may stand and still grab it (0.5 = about 60 degrees). */
	constexpr float AnchorSideMinDot = 0.5f;

	/** True if both grab spots are on the same face of the object. */
	inline bool AreOnSameFace(const FVector& AnchorForwardA, const FVector& AnchorForwardB)
	{
		return FVector::DotProduct(AnchorForwardA.GetSafeNormal2D(), AnchorForwardB.GetSafeNormal2D()) >= SameFaceMinDot;
	}

	/** True if the character stands on the anchor's face of the object. */
	inline bool IsOnAnchorSide(const FVector& AnchorForward, const FVector& ObjectLocation, const FVector& CharacterLocation)
	{
		const FVector ToObject = (ObjectLocation - CharacterLocation).GetSafeNormal2D();
		return FVector::DotProduct(AnchorForward.GetSafeNormal2D(), ToObject) >= AnchorSideMinDot;
	}

	/** Zeroes the push amount if the mode does not allow that direction. +1 push, -1 pull. */
	inline float FilterPushAmount(float PushAmount, EPushPullMode Mode)
	{
		if (PushAmount > 0.f && Mode == EPushPullMode::PullOnly)
		{
			return 0.f;
		}
		if (PushAmount < 0.f && Mode == EPushPullMode::PushOnly)
		{
			return 0.f;
		}
		return PushAmount;
	}

	/** Keeps only the part of Input along Axis, on the ground. Returns zero if Axis is zero. */
	inline FVector ProjectOnAxis(const FVector& Input, const FVector& Axis)
	{
		const FVector FlatAxis = FVector(Axis.X, Axis.Y, 0.f).GetSafeNormal();
		if (FlatAxis.IsNearlyZero())
		{
			return FVector::ZeroVector;
		}
		return FVector::DotProduct(FVector(Input.X, Input.Y, 0.f), FlatAxis) * FlatAxis;
	}
}