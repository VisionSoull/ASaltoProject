// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../Interactables/InteractableBase.h"
#include "Movable.h"
#include "EMovableTypes.h"
#include "MovableBase.generated.h"

class UPushPullAnchorComponent;
class UPushPullComponent;
class AMovableBase;
class UHandGripComponent;
class UCarryComponent;

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

	/** Forward / Back, Left / Right, both without diagonals, or Free. Push Pull Mode only limits the forward / back part. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable")
	EMoveConstraint MoveConstraint = EMoveConstraint::SingleAxis;

	/** Top drag speed in cm/s (GDD: Large 100, Medium 150, Small 200). */	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movable", meta = (ClampMin = 0.0, ForceUnits = "cm/s"))
	float DragSpeed = 150.f;

	/** How hard the movers push, as an acceleration in cm/s^2. Must beat floor friction (about 700 on a default material) and, on a ramp, gravity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable", meta = (ClampMin = 0.0))
	float PushAcceleration = 1500.f;

	/** How quickly the object reaches its drag speed (per second). Lower = smoother start and no speed wobble. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable", meta = (ClampMin = 0.1))
	float PushResponse = 8.f;

	/** Height above the object's lowest point where the push is applied, in cm. Low = it slides up ramps; high = it tips over. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable", meta = (ClampMin = 0.0, Units = "cm"))
	float PushHeight = 5.f;

	/** Below this speed (cm/s) a released object counts as stopped and becomes solid again. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movable", meta = (ClampMin = 0.0, ForceUnits = "cm/s"))
	float RestSpeed = 5.f;

	/** Light only: walking speed multiplier while carrying it (1 = normal speed). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable|Carry", meta = (ClampMin = 0.1, ClampMax = 1.0, EditCondition = "Weight == EMovableWeight::Light", EditConditionHides))
	float CarrySpeedMultiplier = 0.7f;

	/** Very Light only: where the object sits relative to the hand bone that holds it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable|Carry", meta = (EditCondition = "Weight == EMovableWeight::VeryLight", EditConditionHides))
	FVector HandLocationOffset = FVector::ZeroVector;

	/** Very Light only: rotation of the object relative to the hand bone that holds it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable|Carry", meta = (EditCondition = "Weight == EMovableWeight::VeryLight", EditConditionHides))
	FRotator HandRotationOffset = FRotator::ZeroRotator;

	/** Light only: where the object sits relative to the character (X forward, Z up from the capsule centre), in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable|Carry", meta = (EditCondition = "Weight == EMovableWeight::Light", EditConditionHides))
	FVector CarryLocationOffset = FVector(50.f, 0.f, 0.f);

	/** Light only: rotation of the object relative to the character. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable|Carry", meta = (EditCondition = "Weight == EMovableWeight::Light", EditConditionHides))
	FRotator CarryRotationOffset = FRotator::ZeroRotator;

	/** Very Light only: speed the object leaves the hand at when thrown, in cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable|Carry", meta = (ClampMin = 0.0, ForceUnits = "cm/s", EditCondition = "Weight == EMovableWeight::VeryLight", EditConditionHides))
	float ThrowSpeed = 900.f;

	/** Very Light only: how far above the character's facing the throw goes, in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable|Carry", meta = (ClampMin = 0.0, ClampMax = 80.0, Units = "deg", EditCondition = "Weight == EMovableWeight::VeryLight", EditConditionHides))
	float ThrowUpAngle = 20.f;

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

	/** Grips for hand IK. Under: the anchor pushed from, or null for the grips placed directly on the object (carrying). */
	void FindHandGrips(const USceneComponent* Under, UHandGripComponent*& OutLeft, UHandGripComponent*& OutRight) const;

	/** True while a character carries this object (Very Light / Light). */
	UFUNCTION(BlueprintPure, Category = "Movable")
	bool IsCarried() const { return Carrier != nullptr; }

	/** The character carrying this object, or null. */
	UFUNCTION(BlueprintPure, Category = "Movable")
	ACharacter* GetCarrierCharacter() const;

	/** Called by UCarryComponent before attaching: physics off while in the hands. */
	void HandlePickedUp(UCarryComponent* InCarrier);

	/** Called by UCarryComponent after detaching: physics back on. */
	void HandleDropped();

	/** Called by UCarryComponent after detaching: physics back on and launched at LaunchVelocity (cm/s). */
	void HandleThrown(const FVector& LaunchVelocity);

	/** Called by UCarryComponent when Character could not pick this object up. */
	void HandlePickUpFailed(ACharacter* Character, EGrabFailReason Reason);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginPlay() override;
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

	/** Character picked this object up (hide the prompt, affect other interactables...). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Movable")
	void OnPickedUp(ACharacter* Character);

	/** Character put this object down. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Movable")
	void OnDropped(ACharacter* Character);

	/** Character threw this object. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Movable")
	void OnThrown(ACharacter* Character);

	/** Character tried to pick this object up and could not (Reason says why). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Movable")
	void OnPickUpFailed(ACharacter* Character, EGrabFailReason Reason);

	/** Starts locked when true (e.g. already in place). Lock and Unlock change it while playing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movable")
	bool bLocked = false;

private:
	UPushPullComponent* FindMoverOf(const ACharacter* Character) const;
	bool IsMover(const UPushPullComponent* Mover) const;
	bool IsAnchorTaken(const UPushPullAnchorComponent* Anchor) const;
	/** Objects that need several movers keep them on one face. True if Anchor is on the face already held, or nobody holds it. */
	bool IsOnHeldFace(const UPushPullAnchorComponent* Anchor) const;
	void ReleaseAllMovers();
	void StartDrag();
	void StopDrag();

	/** Stops dragging once the object has slowed down to rest (friction does the braking). */
	void StopDragWhenAtRest();

	UPROPERTY(Transient)
	TArray<FMovableMoverSlot> Movers;

	/** Who carries this object (Very Light / Light), or null. */
	UPROPERTY(Transient)
	TObjectPtr<UCarryComponent> Carrier = nullptr;

	bool bDragging = false;
};