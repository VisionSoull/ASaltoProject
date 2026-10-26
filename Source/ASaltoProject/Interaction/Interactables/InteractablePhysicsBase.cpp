// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractablePhysicsBase.h"
#include "../Components/InteractionFeedbackComponent.h"
#include "Components/StaticMeshComponent.h"

AInteractablePhysicsBase::AInteractablePhysicsBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("Root")))
{
	// The mesh is the root, so the actor (and the interaction focus) moves with it
	SetRootComponent(BaseMesh);
	FeedbackComponent->SetupAttachment(BaseMesh);
}
