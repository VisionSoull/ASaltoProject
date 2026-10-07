// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractableBase.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "../Settings/InteractionSettings.h"

AInteractableBase::AInteractableBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create root scene component
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Create interactable mesh
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(RootComponent);
	BaseMesh->SetCollisionProfileName("Interactable");
	BaseMesh->SetComponentTickEnabled(false);
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
}

void AInteractableBase::EndFocus_Implementation(ACharacter* Character, UPrimitiveComponent* FocusedComponent)
{
	bIsFocused = false;
	CurrentlyFocusedComponent = nullptr;
	CurrentlyFocusingCharacter = nullptr;
	UpdateFocusVisual();
}

bool AInteractableBase::CanInteract_Implementation(ACharacter* Character) const
{
	// Default: always can interact
	// Derived classes override for specific conditions
	return true;
}

void AInteractableBase::UpdateFocusVisual()
{
	OnFocusVisualUpdated();
}