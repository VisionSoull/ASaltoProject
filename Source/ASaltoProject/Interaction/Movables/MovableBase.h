// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../Interactables/InteractableBase.h"
#include "Movable.h"
#include "MovableTypes.h"
#include "MovableBase.generated.h"

class UPhysicsHandleComponent;
class UPushPullAnchorComponent;
class UPushPullComponent;
class AMovableBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMovableLockChanged, AMovableBase*, Movable);

/** One character holding this object and the grab spot it uses. */
USTRUCT()
struct FMovableMoverSlot
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UPushPullComponent> Mover = nullptr;

	UPROPERTY()
	TObjectPtr<UPushPullAnchorComponent> Anchor = nullptr;

	/** Already reached its grab spot (OnMoverReady was fired). */
	bool bReady = false;
};

/**
 * A heavy object characters push and pull along the axis of the side they grab.
 * It is an interactable, so focus, outline, widget and Allowed Interactor come from the interaction system.
 * It moves itself with one physics handle, kept at floor height. Lock zones (or Blueprint) lock and unlock it.
 * Add a PushPullAnchor component on every side that can be grabbed, its X arrow pointing into the object.
 */
UCLASS(Blueprintable)
class ASALTOPROJECT_API AMovableBase : public AInteractableBase, public IMovable
{
	GENERATED_BODY()

public:
	AMovableBase(const FObjectInitializer& ObjectInitializer);

	/** Heavy: one character drags it. Very Heavy: two characters must push together. Very Light and Light come later. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable")
	EMovableWeight Weight = EMovableWeight::Heavy;

	/** Hold the button to keep holding, or press once to grab and again to let go. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable")
	EGrabMode GrabMode = EGrabMode::Hold;

	/** Which directions this object accepts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable")
	EPushPullMode PushPullMode = EPushPullMode::PushAndPull;

	/** Single Axis: moves only along the grabbed side's axis. Free: moves in any direction on the ground (Push Pull Mode is ignored). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable")
	EMoveConstraint MoveConstraint = EMoveConstraint::SingleAxis;

	/** Drag speed in cm/s (GDD: Large 100, Medium 150, Small 200). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movable", meta = (ClampMin = 0.0, ForceUnits = "cm/s"))
	float DragSpeed = 150.f;

	/** Fired when the object gets locked (lock zone, Blueprint). */
	UPROPERTY(BlueprintAssignable, Category = "Movable")
	FOnMovableLockChanged OnLocked;

	/** Fired when the object gets unlocked. */
	UPROPERTY(BlueprintAssignable, Category = "Movable")
	FOnMovableLockChanged OnUnlocked;

	/** Locks the object in place. Whoever holds it lets go. */
	UFUNCTION(BlueprintCallable, Category = "Movable")
	void Lock();

	/** Lets characters grab and move the object again. */
	UFUNCTION(BlueprintCallable, Category = "Movable")
	void Unlock();

	/** How many characters hold this object right now. */
	UFUNCTION(BlueprintPure, Category = "Movable")
	int32 GetMoverCount() const { return Movers.Num(); }

	/** True while at least one character holds this object. */
	UFUNCTION(BlueprintPure, Category = "Movable")
	bool IsHeld() const { return Movers.Num() > 0; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	// IInteractable interface
	virtual void BeginInteract_Implementation(ACharacter* Character) override;
	virtual void EndInteract_Implementation(ACharacter* Character) override;

	// IMovable interface
	virtual bool IsLocked_Implementation() const override;
	virtual EMovableWeight GetWeight_Implementation() const override;
	virtual EGrabMode GetGrabMode_Implementation() const override;
	virtual EPushPullMode GetPushPullMode_Implementation() const override;
	virtual UPushPullAnchorComponent* GetFreeAnchorClosestTo_Implementation(const FVector& FromLocation) const override;
	virtual bool RegisterMover_Implementation(UPushPullComponent* Mover, UPushPullAnchorComponent* Anchor) override;
	virtual void UnregisterMover_Implementation(UPushPullComponent* Mover) override;

	/** Started being dragged (sound, dust). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Movable")
	void OnDragStarted();

	/** Stopped being dragged. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Movable")
	void OnDragStopped();

	/** Character grabbed this object and is sliding to its grab spot (e.g. hide the prompt and the outline). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Movable")
	void OnMoverGrabbed(ACharacter* Character);

	/** Character reached its grab spot and can push / pull now. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Movable")
	void OnMoverReady(ACharacter* Character);

	/** Character let go. Focus does not change while holding, so show the prompt and outline again here if IsFocused. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Movable")
	void OnMoverReleased(ACharacter* Character);

	/** The object got locked (lock zone or Lock()). Every mover was already released. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Movable", meta = (DisplayName = "Locked"))
	void ReceiveLocked();

	/** The object got unlocked (Unlock()) and can be grabbed again. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Movable", meta = (DisplayName = "Unlocked"))
	void ReceiveUnlocked();

	/** Starts locked when true (e.g. already in place). Lock and Unlock change it while playing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable")
	bool bLocked = false;

	/** Pulls the object towards the movers' target while dragged. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPhysicsHandleComponent> DragHandle;

private:
	UPushPullComponent* FindMoverOf(const ACharacter* Character) const;
	bool IsMover(const UPushPullComponent* Mover) const;
	bool IsAnchorTaken(const UPushPullAnchorComponent* Anchor) const;
	void ReleaseAllMovers();
	void StartDrag();
	void StopDrag();

	UPROPERTY(Transient)
	TArray<FMovableMoverSlot> Movers;

	bool bDragging = false;

	/** Height kept while dragging (stays on the floor). */
	float DragGroundZ = 0.f;
};