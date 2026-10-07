// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Activable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UActivable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for objects that can be activated/deactivated
 * Supports on/off states, triggers, and conditional activation
 */
class ASALTOPROJECT_API IActivable
{
	GENERATED_BODY()

public:
	/**
	 * Set the active state of this object
	 * @param bActive True to activate, false to deactivate
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Activation")
	void SetActive(bool bActive);

	/** Returns the current active state */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Activation")
	bool IsActive() const;

	/** Returns whether this object can currently be activated */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Activation")
	bool CanActivate() const;

	/** Toggle the active state (activate if inactive, deactivate if active) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Activation")
	void Toggle();

	/** Called when this object becomes active */
	UFUNCTION(BlueprintNativeEvent, Category = "Activation")
	void OnActivated();

	/** Called when this object becomes inactive */
	UFUNCTION(BlueprintNativeEvent, Category = "Activation")
	void OnDeactivated();
};