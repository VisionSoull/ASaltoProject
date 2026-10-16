// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EMovableWeightType.h"
#include "MovableTypes.generated.h"

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
	WeightNotSupported
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

namespace MovableRules
{
	/** Heavy and Very Heavy are grabbed for now; Very Light and Light come later. */
	inline bool IsWeightSupported(EMovableWeight Weight)
	{
		return Weight == EMovableWeight::Heavy || Weight == EMovableWeight::VeryHeavy;
	}

	/** How many characters must push at the same time for the object to move. */
	inline int32 GetRequiredMovers(EMovableWeight Weight)
	{
		return Weight == EMovableWeight::VeryHeavy ? 2 : 1;
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