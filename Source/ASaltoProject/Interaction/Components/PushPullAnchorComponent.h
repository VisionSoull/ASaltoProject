// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PushPullAnchorComponent.generated.h"

/**
 * A spot where a character grabs a movable, one per side.
 * Location: where the character stands. Forward (X arrow): into the object; this is the axis it moves on
 * and the push direction.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ASALTOPROJECT_API UPushPullAnchorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UPushPullAnchorComponent();
};