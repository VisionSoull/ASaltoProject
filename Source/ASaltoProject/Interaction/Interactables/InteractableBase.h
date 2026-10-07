// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "InteractableBase.generated.h"

// Delegate for when interaction completes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteracted, AInteractableBase*, Interactable, ACharacter*, Character);

class UStaticMeshComponent;
class ACharacter;

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

	/** Cached outline material loaded from settings */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> CachedOutlineMaterial = nullptr;

public:
	/** Broadcast when this interactable is successfully interacted with */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteracted OnInteracted;

public:
	AInteractableBase();

protected:
	virtual void BeginPlay() override;

	// IInteractable interface
	virtual void BeginInteract_Implementation(ACharacter* Character) override;
	virtual void EndInteract_Implementation(ACharacter* Character) override;
	virtual void BeginFocus_Implementation(ACharacter* Character, UPrimitiveComponent* FocusedComponent) override;
	virtual void EndFocus_Implementation(ACharacter* Character, UPrimitiveComponent* FocusedComponent) override;
	virtual bool CanInteract_Implementation(ACharacter* Character) const override;

	/** Updates the visual feedback based on current focus state */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Feedback")
	virtual void UpdateFocusVisual();

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnFocusVisualUpdated();

public:
	/** Returns the interactable mesh */
	UStaticMeshComponent* GetInteractableMesh() const { return BaseMesh; }

	/** Returns whether this interactable is currently focused */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsFocused() const { return bIsFocused; }

};