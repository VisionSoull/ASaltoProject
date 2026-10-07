// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractionSettings.h"

UInteractionSettings* UInteractionSettings::GetInteractionSettings()
{
	return GetMutableDefault<UInteractionSettings>();
}