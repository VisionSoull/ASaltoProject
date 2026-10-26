// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "../Movables/EMovableTypes.h"
#include "HandGripComponent.generated.h"

/**
 * Where a character's hand goes on an object. Hand IK reaches for it while pushing, pulling or carrying.
 * Put grips under a PushPullAnchor for that side, or directly on the object for carrying.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ASALTOPROJECT_API UHandGripComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UHandGripComponent();

	/** Which hand uses this grip. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hand Grip")
	EHandSide Side = EHandSide::Left;

	/** Fills the IK outputs from two grips; a missing grip leaves its hand without a target. */
	static void GetGripLocations(const UHandGripComponent* LeftGrip, const UHandGripComponent* RightGrip,
		FVector& OutLeftLocation, FVector& OutRightLocation, bool& bOutHasLeft, bool& bOutHasRight);
};