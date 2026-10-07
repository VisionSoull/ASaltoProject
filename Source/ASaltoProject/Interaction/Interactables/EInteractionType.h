// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * Defines how an interactable responds to interaction input
 */
UENUM(BlueprintType)
enum class EInteractionType : uint8
{
	/** Triggers immediately on press, ignores release */
	Instant UMETA(DisplayName = "Instant (Button)"),

	/** Requires holding for duration, triggers on completion */
	Hold UMETA(DisplayName = "Hold (Lever)"),

	/** Toggles state on/off with each press */
	Toggle UMETA(DisplayName = "Toggle (Switch)")
};