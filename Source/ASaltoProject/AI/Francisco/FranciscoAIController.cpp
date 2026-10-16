// Fill out your copyright notice in the Description page of Project Settings.


#include "FranciscoAIController.h"
#include "FranciscoCharacter.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardAssetProvider.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"

AFranciscoAIController::AFranciscoAIController()
{
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
	BlackboardAsset = CreateDefaultSubobject<UBlackboardData>(TEXT("BlackboardAsset"));
	BehaviorTree = CreateDefaultSubobject<UBehaviorTree>(TEXT("BehaviorTree"));
	
}

void AFranciscoAIController::BeginPlay()
{
	Super::BeginPlay();
	if (BlackboardAsset && BehaviorTree)
	{
		UseBlackboard(BlackboardAsset, BlackboardComponent);
		RunBehaviorTree(BehaviorTree);
	}
}

void AFranciscoAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledFrancisco = Cast<AFranciscoCharacter>(InPawn);

	if (ControlledFrancisco)
	{
		UE_LOG(LogTemp, Log, TEXT("FranciscoAIController possessed Francisco"));
	}
	
}

void AFranciscoAIController::UpdateBlackboard()
{
	if (BlackboardComponent && ControlledFrancisco)
	{
		BlackboardComponent->SetValueAsObject(TEXT("SelfActor"), ControlledFrancisco);
	}
}


