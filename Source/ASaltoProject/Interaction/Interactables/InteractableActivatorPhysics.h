// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractableActivator.h"
#include "InteractableActivatorPhysics.generated.h"

/**
 * An Activator whose BaseMesh is the root component.
 * AInteractableBase builds a separate "Root" scene component, so a mesh that moves on its own
 * (a physics prop carried by a belt) leaves the actor, the interaction focus and the feedback
 * widget behind. Here the mesh is the root, so they all move with it.
 * Everything else comes from AInteractableActivator (ToggleTargets, TargetActivables, cooldown, hold, toggle).
 * "Root" still exists, attached under the mesh, so Blueprint components attached to it keep working.
 */
UCLASS(Blueprintable)
class ASALTOPROJECT_API AInteractableActivatorPhysics : public AInteractableActivator
{
	GENERATED_BODY()

public:
	AInteractableActivatorPhysics();
};
