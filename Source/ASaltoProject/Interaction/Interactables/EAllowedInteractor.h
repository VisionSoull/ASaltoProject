// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EAllowedInteractor.generated.h"

/**
 * Who an interactor is, from the interaction system's point of view.
 * Set on each interactor's UInteractionComponent.
 */
UENUM(BlueprintType)
enum class EInteractorRole : uint8
{
	None,
	Enemy,
	Animal,
	Irene,
	Francisco
};

/**
 * Which interactors an interactable accepts: everyone, or a single role.
 * Checked in CanInteract, and objects a character is not allowed to use never get its focus.
 */
UENUM(BlueprintType)
enum class EAllowedInteractor : uint8
{
	All,
	Enemy,
	Animal,
	Irene,
	Francisco
};

namespace InteractorRules
{
	/** True if an interactable set to Allowed accepts an interactor with Role. */
	inline bool IsRoleAllowed(EAllowedInteractor Allowed, EInteractorRole Role)
	{
		switch (Allowed)
		{
		case EAllowedInteractor::All:       return true;
		case EAllowedInteractor::Enemy:     return Role == EInteractorRole::Enemy;
		case EAllowedInteractor::Animal:    return Role == EInteractorRole::Animal;
		case EAllowedInteractor::Irene:     return Role == EInteractorRole::Irene;
		case EAllowedInteractor::Francisco: return Role == EInteractorRole::Francisco;
		}
		return false;
	}
}