// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "EAllowedInteractor.h"
#include "InteractableBase.generated.h"

// Delegate for when interaction completes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteracted, AInteractableBase*, Interactable, ACharacter*, Character);

class UStaticMeshComponent;
class ACharacter;
class UInteractionFeedbackComponent;

/**
 * Minimal base class for all interactable objects in The Operator
 * Provides focus/outline and basic interaction contract
 * Specific interaction behaviors implemented in derived classes
 */
UCLASS(Abstract, Blueprintable)
class ASALTOPROJECT_API AInteractableBase : public AActor, public IInteractable
{
	GENERATED_BODY()

protected:
	/** Visual mesh for the interactable */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	/** Is this interactable currently being focused/aimed at? */
	bool bIsFocused = false;

	/** The component currently receiving focus */
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> CurrentlyFocusedComponent = nullptr;

	/** The character currently focusing on this interactable */
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> CurrentlyFocusingCharacter = nullptr;

	/** Feedback toolbox (prompt widget + highlight). Its functions are called from this object's Blueprint. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInteractionFeedbackComponent> FeedbackComponent;

public:
	/** Broadcast when this interactable is successfully interacted with */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteracted OnInteracted;

	/** Which interactors can use this: everyone or a single role. Compared against the interactor's UInteractionComponent role. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	EAllowedInteractor AllowedInteractor = EAllowedInteractor::All;

public:
	AInteractableBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;

	// IInteractable interface
	virtual void BeginInteract_Implementation(ACharacter* Character) override;
	virtual void EndInteract_Implementation(ACharacter* Character) override;
	virtual void BeginFocus_Implementation(ACharacter* Character, UPrimitiveComponent* FocusedComponent) override;
	virtual void EndFocus_Implementation(ACharacter* Character, UPrimitiveComponent* FocusedComponent) override;
	virtual bool CanInteract_Implementation(ACharacter* Character) const override;
	virtual void BeginRange_Implementation(ACharacter* Character) override;
	virtual void EndRange_Implementation(ACharacter* Character) override;

	/** This object entered Character's interaction range. Decide here what to show (e.g. FeedbackComponent->ShowWidget). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnRangeEntered(ACharacter* Character);

	/** This object left Character's interaction range. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnRangeExited(ACharacter* Character);

	/** Character is now focusing this object; FocusedComponent is the primitive that was aimed at. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnFocusGained(ACharacter* Character, UPrimitiveComponent* FocusedComponent);

	/** Character stopped focusing this object. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnFocusLost(ACharacter* Character);

	/** Updates the visual feedback based on current focus state */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Feedback")
	virtual void UpdateFocusVisual();

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnFocusVisualUpdated();

public:
	/** Returns the interactable mesh */
	UStaticMeshComponent* GetInteractableMesh() const { return BaseMesh; }

	/** Returns the feedback toolbox (prompt widget + highlight) */
	UFUNCTION(BlueprintPure, Category = "Interaction|Feedback")
	UInteractionFeedbackComponent* GetFeedbackComponent() const { return FeedbackComponent; }

	/** Returns whether this interactable is currently focused */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsFocused() const { return bIsFocused; }

	/** True if AllowedInteractor accepts this character. Used by CanInteract and by the focus selection. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsInteractorAllowed(const ACharacter* Character) const;
};