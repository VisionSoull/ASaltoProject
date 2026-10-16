// Fill out your copyright notice in the Description page of Project Settings.


#include "FranciscoCharacter.h"
#include "FranciscoAIController.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "InputAction.h"

// Sets default values
AFranciscoCharacter::AFranciscoCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ProximityComponent = CreateDefaultSubobject<UProximityComponent>(TEXT("ProximityComponent"));
	EmotionalState = CreateDefaultSubobject<UEmotionalStateComponent>(TEXT("EmotionalState"));
	
	AIControllerClass = AFranciscoAIController::StaticClass();

	PlayerReference = nullptr;
	FranciscoAI = nullptr;

	GetCapsuleComponent()->SetCapsuleHalfHeight(88.0f);
	GetCapsuleComponent()->SetCapsuleRadius(34.0f);

	GetCharacterMovement()->MaxWalkSpeed = 300.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

}

// Called when the game starts or when spawned
void AFranciscoCharacter::BeginPlay()
{
	Super::BeginPlay();

	FranciscoAI = Cast<AFranciscoAIController>(GetController());

	if (FranciscoAI)
	{
		UE_LOG(LogTemp, Log, TEXT("Francisco connected to AI Controller"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Francisco: AI Controller not found!"));
	}
	
	FindPlayerReference();
	
	UE_LOG(LogTemp, Log, TEXT("Francisco Character initialized"));
   
}

// Called every frame
void AFranciscoCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AFranciscoCharacter::SetPlayerReference(APawn* NewPlayerReference)
{
	PlayerReference = NewPlayerReference;
    
	if (PlayerReference)
	{
		UE_LOG(LogTemp, Log, TEXT("Francisco: Player reference set to %s"), *PlayerReference->GetName());
	}
}

void AFranciscoCharacter::FindPlayerReference()
{
	// Simple method: find first player-controlled pawn
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			if (APlayerController* PC = Iterator->Get())
			{
				if (APawn* PlayerPawn = PC->GetPawn())
				{
					SetPlayerReference(PlayerPawn);
					break;
				}
			}
		}
	}
    
	if (!PlayerReference)
	{
		UE_LOG(LogTemp, Warning, TEXT("Francisco: Could not find player reference"));
	}
}

float AFranciscoCharacter::GetDistanceToPlayer() const
{
	return ProximityComponent ? ProximityComponent->GetDistanceToPlayer() : 0.0f;
}

float AFranciscoCharacter::GetStressLevel() const
{
	return EmotionalState ? EmotionalState->GetTotalStress() : 0.0f;
}

EEmotionalState AFranciscoCharacter::GetEmotionalState() const
{
	return EmotionalState ? EmotionalState->GetEmotionalState() : EEmotionalState::Calm;
}

void AFranciscoCharacter::TestComponentCommunication()
{
	if (ProximityComponent && EmotionalState)
	{
		UE_LOG(LogTemp, Log, TEXT("=== COMPONENT STATUS ==="));
		UE_LOG(LogTemp, Log, TEXT("Distance to Player: %.1f"), GetDistanceToPlayer());
		UE_LOG(LogTemp, Log, TEXT("Proximity Zone: %s"), *UEnum::GetValueAsString(ProximityComponent->GetCurrentZone()));
		UE_LOG(LogTemp, Log, TEXT("Stress Level: %.1f"), GetStressLevel());
		UE_LOG(LogTemp, Log, TEXT("Emotional State: %s"), *UEnum::GetValueAsString(GetEmotionalState()));
		UE_LOG(LogTemp, Log, TEXT("========================"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Some components are missing!"));
	}
}

void AFranciscoCharacter::OnHoldHandAction(const FInputActionInstance& Instance)
{
    UE_LOG(LogTemp, Log, TEXT("HoldHand action triggered"));
    // Add your logic here
}

void AFranciscoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // Make sure HoldHandInputAction is a valid UInputAction* property of your class
        EnhancedInput->BindAction(HoldHandInputAction, ETriggerEvent::Triggered, this, &AFranciscoCharacter::OnHoldHandAction);
    }
}
