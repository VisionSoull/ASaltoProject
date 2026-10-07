// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class ACharacter;
class UPrimitiveComponent;

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for objects that can be interacted with by both VR and FP characters
 * Supports outline visualization and interaction events
 */
class ASALTOPROJECT_API IInteractable
{
	GENERATED_BODY()

public:
	/**
	 * Called when character begins interacting (e.g., E key pressed)
	 * 
	 * WARNING: Do NOT override this in Blueprint! Use OnInteractBegan event instead.
	 * This function manages critical state (cooldown, timing, used state).
	 * Overriding will break the interaction system.
	 * 
	 * @param Character The character initiating the interaction
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void BeginInteract(ACharacter* Character);

	/**
	 * Called when character ends interacting (e.g., E key released)
	 * 
	 * WARNING: Do NOT override this in Blueprint! Use OnInteractEnded event instead.
	 * This function manages critical state (cooldown, timing, used state).
	 * Overriding will break the interaction system.
	 * 
	 * @param Character The character ending the interaction
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void EndInteract(ACharacter* Character);

	/**
	 * Called when this interactable receives player focus (aimed at)
	 * @param FocusedComponent The specific component that was aimed at
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void BeginFocus(ACharacter* Character, UPrimitiveComponent* FocusedComponent);

	/**
	 * Called when this interactable loses player focus (no longer aimed at)
	 * @param FocusedComponent The component that was previously focused
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void EndFocus(ACharacter* Character, UPrimitiveComponent* FocusedComponent);

	/**
	 * Returns whether this interactable can currently be interacted with
	 * @param Character The character attempting to interact
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(ACharacter* Character) const;
};