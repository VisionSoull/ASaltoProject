// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractableBase.h"
#include "InteractablePhysicsBase.generated.h"

/**
 * An interactable whose BaseMesh is the root component.
 * AInteractableBase keeps a separate "Root" scene component, so a mesh that moves on its own
 * (a physics prop carried by a belt, a rolling object) leaves the actor, the interaction focus
 * and the feedback widget behind. Here the mesh is the root, so they all move with it.
 * Use it as the parent of Blueprints for objects that move. Static interactables keep AInteractableBase.
 */
UCLASS(Abstract, Blueprintable)
class ASALTOPROJECT_API AInteractablePhysicsBase : public AInteractableBase
{
	GENERATED_BODY()

public:
	AInteractablePhysicsBase(const FObjectInitializer& ObjectInitializer);
};
