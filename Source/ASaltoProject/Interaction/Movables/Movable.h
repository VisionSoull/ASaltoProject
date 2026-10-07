// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
 #include "EMovableWeightType.h" 
#include "Movable.generated.h"       

class UPushPullComponent;            

UINTERFACE(MinimalAPI, Blueprintable)
class UMovable : public UInterface
{
    GENERATED_BODY()
};

class ASALTOPROJECT_API IMovable
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Movable")
    bool CanBeMoved() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Movable")
    EMovableWeight GetWeight() const;

    // Regista/retira um mover. Devolve true se o pedido foi aceite.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Movable")
    bool RegisterMover(UPushPullComponent* Mover);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Movable")
    void UnregisterMover(UPushPullComponent* Mover);
};