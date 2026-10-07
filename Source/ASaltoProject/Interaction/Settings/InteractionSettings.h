// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InteractionSettings.generated.h"

/**
 * Project-wide settings for the interaction system
 * Accessible via Project Settings -> Game -> Interaction Settings
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Interaction Settings"))
class ASALTOPROJECT_API UInteractionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Default cooldown time between interactions (can be overwritten per-actor)*/
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = 0.0, ClampMax = 5.0, Units = "s"))
	float DefaultCooldown = 0.5f;

	/** Default hold duration for hold-type interactions (can be overwritten per-actor)*/
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = 0.1, ClampMax = 5.0, Units = "s"))
	float DefaultHoldDuration = 2.0f;

	/** Blueprint-accessible function to get the settings instance */
	UFUNCTION(BlueprintPure, Category = "Settings", meta = (DisplayName = "Get Interaction Settings"))
	static UInteractionSettings* GetInteractionSettings();
};
