// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Movables/MovableTypes.h"
#include "PushPullComponent.generated.h"

class ACharacter;
class UPushPullAnchorComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPushPullObjectEvent, AActor*, Object);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPushPullGrabFailed, AActor*, Object, EGrabFailReason, Reason);

/**
 * A character's side of push/pull. Grabs an IMovable, slides the character to the closest grab spot,
 * keeps it there and turns the movement input into a push amount the object uses to move itself.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ASALTOPROJECT_API UPushPullComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPushPullComponent();

	/** Seconds to slide into the grab spot before pushing starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PushPull", meta = (ClampMin = 0.0, ClampMax = 2.0, Units = "s"))
	float AimingDuration = 0.2f;

	UPROPERTY(BlueprintAssignable, Category = "PushPull")
	FOnPushPullObjectEvent OnGrabbed;

	UPROPERTY(BlueprintAssignable, Category = "PushPull")
	FOnPushPullObjectEvent OnReleased;

	UPROPERTY(BlueprintAssignable, Category = "PushPull")
	FOnPushPullGrabFailed OnGrabFailed;

	/** The character reached its grab spot and can push / pull from now on. */
	UPROPERTY(BlueprintAssignable, Category = "PushPull")
	FOnPushPullObjectEvent OnReadyToPush;

	/** Tries to grab Object (an IMovable). Fires OnGrabbed or OnGrabFailed. */
	UFUNCTION(BlueprintCallable, Category = "PushPull")
	bool TryGrab(AActor* Object);

	/** Lets go of the held object, if any. */
	UFUNCTION(BlueprintCallable, Category = "PushPull")
	void Release();

	/** Drives this mover from code instead of the player's movement input (AI, commands, tests). World space. */
	UFUNCTION(BlueprintCallable, Category = "PushPull")
	void SetMoveInput(FVector Input);

	/** Goes back to the player's movement input. */
	UFUNCTION(BlueprintCallable, Category = "PushPull")
	void ClearMoveInput();

	/** The movement input this mover uses this frame, in world space. */
	UFUNCTION(BlueprintPure, Category = "PushPull")
	FVector GetMoveInput() const;

	UFUNCTION(BlueprintPure, Category = "PushPull")
	AActor* GetHeldObject() const { return HeldObject; }

	UFUNCTION(BlueprintPure, Category = "PushPull")
	bool IsHolding() const { return HeldObject != nullptr; }

	/** True once the character sits at its grab spot; only then does its push amount move the object. */
	UFUNCTION(BlueprintPure, Category = "PushPull")
	bool IsAligned() const { return bIsAligned; }

	/** +1 pushing, -1 pulling, 0 still. 0 for a direction the object does not accept. For animation (Blend Space). */
	UFUNCTION(BlueprintPure, Category = "PushPull")
	float GetPushAmount() const { return PushAmount; }

	/** Movement input on the ground (dead zone applied, length up to 1). Free-moving objects follow it. */
	UFUNCTION(BlueprintPure, Category = "PushPull")
	FVector GetDragInput() const { return DragInput; }

	/** How fast push/pull moves the character (sliding to the grab spot, then following the object), in cm/s.
	*  CharacterMovement's velocity stays 0 while holding, so the Anim BP reads this instead. */
	UFUNCTION(BlueprintPure, Category = "PushPull")
	float GetMoveSpeed() const { return MoveSpeed; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void FailGrab(AActor* Object, EGrabFailReason Reason);

	/** Stops the character walking on its own (its input drives the object instead), or restores it. */
	void LockMovement(bool bLock);

	/** Keeps the interaction focus on the held object, or frees it. */
	void LockFocus(bool bLock);

	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AActor> HeldObject = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPushPullAnchorComponent> HeldAnchor = nullptr;

	bool bIsAligned = false;
	float PushAmount = 0.f;

	FVector DragInput = FVector::ZeroVector;

	float MoveSpeed = 0.f;
	FVector LastLocation = FVector::ZeroVector;

	FVector ExternalInput = FVector::ZeroVector;
	bool bHasExternalInput = false;

	FVector AimStartLocation = FVector::ZeroVector;
	FRotator AimStartRotation = FRotator::ZeroRotator;
	float AimElapsed = 0.f;

	/** Movement values saved by LockMovement. Below 0 = nothing saved. */
	float SavedMaxWalkSpeed = -1.f;
	bool bSavedOrientRotationToMovement = false;
};