// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Activable.h"
#include "ActivableBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActivationStateChanged, bool, bNewState, AActivableBase*, Activable);

/**
 * Base class for all activable objects in The Operator
 * Provides flexible activation mechanics that Blueprint children configure for specific gameplay
 */
UCLASS(Abstract, Blueprintable)
class ASALTOPROJECT_API AActivableBase : public AActor, public IActivable
{
	GENERATED_BODY()

protected:
	/** Is this activable currently in the active state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsActive = false;

	/** Is this activable currently busy and unable to accept new commands */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsBusy = false;

	/** Can this activable be activated/deactivated multiple times, or only once? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activation")
	bool bCanToggleMultipleTimes = true;

	/** Has this activable been activated at least once (for single-use activables) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bHasBeenActivated = false;

	/** Cooldown time after activation before it can be activated again */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activation", meta = (ClampMin = 0.0, ClampMax = 10.0, Units = "s"))
	float ActivationCooldown = 0.0f;

	/** Is this activable currently on cooldown */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsOnCooldown = false;

	/** Timer for cooldown */
	FTimerHandle CooldownTimer;

public:
	/** Broadcast when activation state changes */
	UPROPERTY(BlueprintAssignable, Category = "Activation")
	FOnActivationStateChanged OnActivationStateChanged;

public:
	AActivableBase();

protected:
	virtual void BeginPlay() override;

	// IActivable interface
	virtual void SetActive_Implementation(bool bActive) override;
	virtual bool IsActive_Implementation() const override;
	virtual bool CanActivate_Implementation() const override;
	virtual void Toggle_Implementation() override;
	virtual void OnActivated_Implementation() override;
	virtual void OnDeactivated_Implementation() override;

	/** Called when cooldown timer expires */
	void OnCooldownExpired();

public:
	// Blueprint callable helpers

	/** Set busy state - when true, blocks new activations */
	UFUNCTION(BlueprintCallable, Category = "Activation")
	void SetBusy(bool bNewBusy);

	/** Returns whether this activable is currently busy */
	UFUNCTION(BlueprintPure, Category = "Activation")
	bool IsBusy() const { return bIsBusy; }

	/** Reset this activable (clears used state, cooldown, busy, etc.) */
	UFUNCTION(BlueprintCallable, Category = "Activation")
	void ResetActivable();

	/** Manually set the cooldown state */
	UFUNCTION(BlueprintCallable, Category = "Activation")
	void SetCooldownActive(bool bActive);
};