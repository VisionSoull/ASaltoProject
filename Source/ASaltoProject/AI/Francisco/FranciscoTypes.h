#pragma once

#include "CoreMinimal.h"
#include "FranciscoTypes.generated.h"

UENUM(BlueprintType)
enum class EFranciscoState : uint8
{
	Connected UMETA(DisplayName = "Connected"),
	Disconnected UMETA(DisplayName = "Disconnected"),
	Commanded UMETA(DisplayName = "Commnanded"),
	Panicked UMETA(DisplayName = "Panicked")
};

UENUM(BlueprintType)
enum class EProximityZone : uint8
{
	Comfort     UMETA(DisplayName = "Comfort Zone"),     
	Normal      UMETA(DisplayName = "Normal Zone"),      
	Warning     UMETA(DisplayName = "Warning Zone"),     
	Danger      UMETA(DisplayName = "Danger Zone"),      
	Panic       UMETA(DisplayName = "Panic Zone")       
};

UENUM(BlueprintType)
enum class EStressSource : uint8
{
	Distance        UMETA(DisplayName = "Distance from Player")
};

UENUM(BlueprintType)
enum class EEmotionalState : uint8
{
	Calm        UMETA(DisplayName = "Calm"),
	Nervous     UMETA(DisplayName = "Nervous"),
	Anxious     UMETA(DisplayName = "Anxious"),
	Panicked    UMETA(DisplayName = "Panicked")
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFranciscoStateChanged, EFranciscoState, NewState, EFranciscoState, OldState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProximityChanged, float, OldDistance, float, NewDistance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZoneChanged, EProximityZone, NewZone);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStressChanged, float, OldStress, float, NewStress);

