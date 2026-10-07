// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EMovableWeight : uint8
{
    VeryLight UMETA(DisplayName = "Very Light"),
    Light     UMETA(DisplayName = "Light"),
    Heavy     UMETA(DisplayName = "Heavy"),
    VeryHeavy UMETA(DisplayName = "Very Heavy")
};
