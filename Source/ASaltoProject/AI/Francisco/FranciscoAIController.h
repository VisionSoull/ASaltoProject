// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "FranciscoAIController.generated.h"

class AFranciscoCharacter;
class UBehaviorTree;
class UBlackboardData;
class UBlackboardComponent;

UCLASS()
class ASALTOPROJECT_API AFranciscoAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFranciscoAIController();
protected:
	virtual void BeginPlay() override;	
	virtual void OnPossess(APawn* InPawn) override;
	
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	UBehaviorTree* BehaviorTree;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	UBlackboardData* BlackboardAsset;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	UBlackboardComponent* BlackboardComponent;
private:
	UPROPERTY()
	AFranciscoCharacter* ControlledFrancisco;
	
public:
	AFranciscoCharacter* GetControlledFrancisco() const { return ControlledFrancisco; }
	void UpdateBlackboard();
};
