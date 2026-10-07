// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractableCommon.h"
#include "InteractableBroadcaster.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionBroadcast, bool, bIsActive, AInteractableBroadcaster*, Broadcaster);

/**
 * Interactable that broadcasts interaction events.
 */
UCLASS(Blueprintable)
class ASALTOPROJECT_API AInteractableBroadcaster : public AInteractableCommon
{
	GENERATED_BODY()

protected:
	/**
	 * Should broadcasts happen automatically on interaction executed, or manually via Blueprint?
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Broadcasting")
	bool bAutoBroadcast = true;

public:
	/**
	 * Broadcast when this interactable's condition state changes.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractionBroadcast OnInteractionBroadcast;

public:
	/**
	 * Manually broadcast state
	 * Safe to call from Blueprint on all machines - only executes on server.
	 */
	UFUNCTION(BlueprintCallable, Category = "Broadcasting")
	void BroadcastState(bool bNewState);

protected:
	/** Server: Auto-broadcast on interaction (if bAutoBroadcast is true) */
	virtual void OnInteractionExecuted_Server_Implementation(ACharacter* Character) override;

	/** Server: Auto-broadcast on hold (if bAutoBroadcast is true) */
	virtual void OnHoldReleased_Server_Implementation(ACharacter* Character) override;

	/** Server: Auto-broadcast on toggle change (if bAutoBroadcast is true) */
	virtual void OnToggleChanged_Server_Implementation(bool bNewState, ACharacter* Character) override;
};