// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractableBase.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "../Settings/InteractionSettings.h"
#include "../Components/InteractionComponent.h"
#include "../Components/InteractionFeedbackComponent.h"

AInteractableBase::AInteractableBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	// Optional so subclasses whose mesh must be the root (physics movables) can skip it
	USceneComponent* Root = CreateOptionalDefaultSubobject<USceneComponent>(TEXT("Root"));
	if (Root)
	{
		SetRootComponent(Root);
	}

	// Create interactable mesh
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(RootComponent);
	BaseMesh->SetCollisionProfileName("Interactable");
	BaseMesh->SetComponentTickEnabled(false);

	// Empty until the Blueprint calls ShowWidget; profile chosen per object or from Interaction Settings
	FeedbackComponent = CreateDefaultSubobject<UInteractionFeedbackComponent>(TEXT("InteractionFeedback"));
	FeedbackComponent->SetupAttachment(RootComponent);
}

void AInteractableBase::BeginPlay()
{
	Super::BeginPlay();
}

void AInteractableBase::BeginInteract_Implementation(ACharacter* Character)
{
	// Default: empty
	// Simple interactables (food, climbable) override this in Blueprint
}

void AInteractableBase::EndInteract_Implementation(ACharacter* Character)
{
	// Default: empty
	// Simple interactables override this in Blueprint if needed
}

void AInteractableBase::BeginFocus_Implementation(ACharacter* Character, UPrimitiveComponent* FocusedComponent)
{
	bIsFocused = true;
	CurrentlyFocusedComponent = FocusedComponent;
	CurrentlyFocusingCharacter = Character;
	UpdateFocusVisual();
	OnFocusGained(Character, FocusedComponent);
}

void AInteractableBase::EndFocus_Implementation(ACharacter* Character, UPrimitiveComponent* FocusedComponent)
{
	bIsFocused = false;
	CurrentlyFocusedComponent = nullptr;
	CurrentlyFocusingCharacter = nullptr;
	UpdateFocusVisual();
	OnFocusLost(Character);
}

void AInteractableBase::BeginRange_Implementation(ACharacter* Character)
{
	OnRangeEntered(Character);
}

void AInteractableBase::EndRange_Implementation(ACharacter* Character)
{
	OnRangeExited(Character);
}

bool AInteractableBase::CanInteract_Implementation(ACharacter* Character) const
{
	// Derived classes add their own conditions on top of this
	return IsInteractorAllowed(Character);
}

bool AInteractableBase::IsInteractorAllowed(const ACharacter* Character) const
{
	const UInteractionComponent* InteractionComp = Character ? Character->FindComponentByClass<UInteractionComponent>() : nullptr;
	const EInteractorRole InteractorRole = InteractionComp ? InteractionComp->InteractorRole : EInteractorRole::None;
	return InteractorRules::IsRoleAllowed(AllowedInteractor, InteractorRole);
}

void AInteractableBase::UpdateFocusVisual()
{
	OnFocusVisualUpdated();
}