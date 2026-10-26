// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EMovableWeightType.h"
#include "EMovableTypes.h"
#include "Movable.generated.h"

class UPushPullComponent;
class UPushPullAnchorComponent;

UINTERFACE(MinimalAPI, Blueprintable)
class UMovable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Push/pull contract, parallel to IInteractable.
 * The object moves itself: movers only register, give their push amount and unregister.
 * Always call through IMovable::Execute_<Function>(Actor, ...).
 */
class ASALTOPROJECT_API IMovable
{
	GENERATED_BODY()

public:
	/** True while the object is locked (lock zone, puzzle, cutscene). Locked objects cannot be grabbed. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Movable")
	bool IsLocked() const;

	/** Only Heavy can be grabbed for now. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Movable")
	EMovableWeight GetWeight() const;

	/** Hold the button to keep holding, or press once to grab and again to let go. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Movable")
	EGrabMode GetGrabMode() const;

	/** Which directions the object accepts: push only, pull only, or both. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Movable")
	EPushPullMode GetPushPullMode() const;

	/** Free grab spot closest to FromLocation, or null if all are taken. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Movable")
	UPushPullAnchorComponent* GetFreeAnchorClosestTo(const FVector& FromLocation) const;

	/** Adds a mover at Anchor. Returns false if refused. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Movable")
	bool RegisterMover(UPushPullComponent* Mover, UPushPullAnchorComponent* Anchor);

	/** Removes a mover. Does nothing if it was not registered. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Movable")
	void UnregisterMover(UPushPullComponent* Mover);
};