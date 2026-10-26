// Fill out your copyright notice in the Description page of Project Settings.

#include "HandGripComponent.h"

UHandGripComponent::UHandGripComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA
	// Draw the arrow so designers see where the hand grabs
	bVisualizeComponent = true;
#endif
}

void UHandGripComponent::GetGripLocations(const UHandGripComponent* LeftGrip, const UHandGripComponent* RightGrip,
	FVector& OutLeftLocation, FVector& OutRightLocation, bool& bOutHasLeft, bool& bOutHasRight)
{
	bOutHasLeft = IsValid(LeftGrip);
	bOutHasRight = IsValid(RightGrip);
	OutLeftLocation = bOutHasLeft ? LeftGrip->GetComponentLocation() : FVector::ZeroVector;
	OutRightLocation = bOutHasRight ? RightGrip->GetComponentLocation() : FVector::ZeroVector;
}