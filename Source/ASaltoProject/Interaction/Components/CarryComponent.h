// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Movables/EMovableTypes.h"
#include "CarryComponent.generated.h"

class ACharacter;
class AMovableBase;
class UHandGripComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCarryObjectEvent, AMovableBase*, Object);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCarryFailed, AMovableBase*, Object, EGrabFailReason, Reason);

/**
 * A character's side of carrying. Very Light objects go in the closer hand, Light ones in front of the character.
 * Tap interact to drop; hold it and release to throw a Very Light object.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ASALTOPROJECT_API UCarryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCarryComponent();

	/** Hand bones Very Light objects are held in. The closer one at pick-up is used. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carry")
	FName LeftHandBone = TEXT("LeftHand");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carry")
	FName RightHandBone = TEXT("RightHand");

	/** Holding interact at least this long throws a Very Light object instead of dropping it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carry", meta = (ClampMin = 0.05, ClampMax = 2.0, Units = "s"))
	float ThrowHoldSeconds = 0.35f;

	/** How far in front of the character a dropped object is put down, in cm. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carry", meta = (ClampMin = 0.0, Units = "cm"))
	float DropDistance = 60.f;

	UPROPERTY(BlueprintAssignable, Category = "Carry")
	FOnCarryObjectEvent OnPickedUp;

	UPROPERTY(BlueprintAssignable, Category = "Carry")
	FOnCarryObjectEvent OnDropped;

	UPROPERTY(BlueprintAssignable, Category = "Carry")
	FOnCarryObjectEvent OnThrown;

	UPROPERTY(BlueprintAssignable, Category = "Carry")
	FOnCarryFailed OnPickUpFailed;

	/** Picks Object up (Very Light or Light). Fires OnPickedUp or OnPickUpFailed. */
	UFUNCTION(BlueprintCallable, Category = "Carry")
	bool TryPickUp(AMovableBase* Object);

	/** Puts the carried object down in front of the character. */
	UFUNCTION(BlueprintCallable, Category = "Carry")
	void Drop();

	/** Throws the carried object if it is Very Light; drops it otherwise. */
	UFUNCTION(BlueprintCallable, Category = "Carry")
	void Throw();

	/** Interact pressed while carrying: starts timing the hold. */
	void BeginRelease();

	/** Interact released while carrying: throws after a long enough hold (Very Light), drops otherwise. */
	void FinishRelease();

	UFUNCTION(BlueprintPure, Category = "Carry")
	bool IsCarrying() const { return CarriedObject != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Carry")
	AMovableBase* GetCarriedObject() const { return CarriedObject; }

	/** 0..1 while interact is held towards a throw, 0 otherwise. For a throw charge UI. */
	UFUNCTION(BlueprintPure, Category = "Carry")
	float GetThrowHoldProgress() const;

	/** World positions for hand IK while carrying a Light object: its hand grips. A hand without a grip gets false. */
	UFUNCTION(BlueprintPure, Category = "Carry")
	void GetHandTargets(FVector& OutLeftLocation, FVector& OutRightLocation, bool& bOutHasLeft, bool& bOutHasRight) const;

	/** World rotations of those grips, to turn the hands. Zero for a hand without a grip. */
	UFUNCTION(BlueprintPure, Category = "Carry")
	void GetHandTargetRotations(FRotator& OutLeftRotation, FRotator& OutRightRotation) const;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void FailPickUp(AMovableBase* Object, EGrabFailReason Reason);

	/** Detaches the carried object and restores the character. Returns the object, or null if nothing was carried. */
	AMovableBase* DetachCarried();

	/** Slows walking while carrying a Light object, or restores it. */
	void SetMovementSlowed(bool bSlowed, float SpeedMultiplier);

	/** Keeps the interaction focus on the carried object, so interact presses reach it. */
	void SetFocusLocked(bool bLocked);

	/** Grips placed directly on the carried object, when it is carried with both hands (Light). Null otherwise. */
	void FindCarriedGrips(UHandGripComponent*& OutLeft, UHandGripComponent*& OutRight) const;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AMovableBase> CarriedObject = nullptr;

	/** World time interact was pressed while carrying; below 0 = no drop / throw in progress. */
	double ReleasePressTime = -1.0;

	/** Walk speed saved while slowed by a Light object. Below 0 = nothing saved. */
	float SavedMaxWalkSpeed = -1.f;
};