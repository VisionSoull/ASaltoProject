// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LogicGateBase.h"
#include "LogicGateAND.generated.h"

class AInteractableActivator;

/**
 * AND Logic Gate - activates targets when ALL conditions are met
 */
UCLASS()
class ASALTOPROJECT_API ALogicGateAND : public ALogicGateBase
{
	GENERATED_BODY()

protected:
	/** Check if ALL required conditions are met */
	virtual bool AreAllConditionsMet() const override;
};