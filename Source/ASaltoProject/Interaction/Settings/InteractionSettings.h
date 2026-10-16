// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InteractionSettings.generated.h"

class UInteractionFeedbackProfile;

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

	/** Outline and widget used by every interactable that does not pick its own profile */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TSoftObjectPtr<UInteractionFeedbackProfile> DefaultFeedbackProfile;

	/** A mover lets go when it ends up farther than this from its grab spot (object stuck), in cm */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Push Pull", meta = (ClampMin = 10.0, Units = "cm"))
	float PushPullBreakDistance = 60.0f;

	/** How much movement input must point along the drag direction to count as pushing (0..1) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Push Pull", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float PushPullInputDeadZone = 0.2f;

	/** Blueprint-accessible function to get the settings instance */
	UFUNCTION(BlueprintPure, Category = "Settings", meta = (DisplayName = "Get Interaction Settings"))
	static UInteractionSettings* GetInteractionSettings();
};
