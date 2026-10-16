#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CameraPIESaveLibrary.generated.h"

UCLASS()
class ASALTOPROJECT_API UCameraPIESaveLibrary
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Camera|Debug")
    static bool SavePIEActorToEditor(
        AActor* PIEActor,
        FString& OutMessage
    );
};
