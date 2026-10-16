// Fill out your copyright notice in the Description page of Project Settings.

#include "PushPullAnchorComponent.h"

UPushPullAnchorComponent::UPushPullAnchorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA
	// Draw the arrow so designers see where the character stands and which way it pushes
	bVisualizeComponent = true;
#endif
}