// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractableCommon.h"
#include "InteractableActivator.generated.h"

/**
 * Interactable that directly activates/deactivates target Activables.
 */
UCLASS()
class ASALTOPROJECT_API AInteractableActivator : public AInteractableCommon
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activation")
	TArray<AActor*> TargetActivables;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activation")
	bool bAutoActivateTargets = true;

	// ==================== Server-Only Logic ====================

	virtual void OnInteractionExecuted_Server_Implementation(ACharacter* Character) override;
	virtual void OnToggleChanged_Server_Implementation(bool bNewState, ACharacter* Character) override;
	virtual void OnHoldReleased_Server_Implementation(ACharacter* Character) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Activation", meta = (BlueprintAuthorityOnly))
	void ActivateTargets();

	UFUNCTION(BlueprintCallable, Category = "Activation", meta = (BlueprintAuthorityOnly))
	void DeactivateTargets();

	UFUNCTION(BlueprintCallable, Category = "Activation", meta = (BlueprintAuthorityOnly))
	void ToggleTargets();
};