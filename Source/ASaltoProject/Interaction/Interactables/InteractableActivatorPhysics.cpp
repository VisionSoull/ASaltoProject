// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractableActivatorPhysics.h"
#include "../Components/InteractionFeedbackComponent.h"
#include "Components/StaticMeshComponent.h"

AInteractableActivatorPhysics::AInteractableActivatorPhysics()
{
	// The base classes already built "Root" with BaseMesh under it. Swap them: the mesh becomes
	// the root, so the actor (and the interaction focus) moves with it. Root is kept, riding on the mesh.
	USceneComponent* OldRoot = GetRootComponent();

	BaseMesh->SetupAttachment(nullptr);
	SetRootComponent(BaseMesh);

	if (OldRoot && OldRoot != BaseMesh)
	{
		OldRoot->SetupAttachment(BaseMesh);
	}

	FeedbackComponent->SetupAttachment(BaseMesh);
}
