// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MovableLockZone.generated.h"

class UBoxComponent;
class USceneComponent;
class UPrimitiveComponent;
class AMovableBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZoneMovableLocked, AMovableBase*, Movable);

/**
 * Drag a movable into this area: once it gets within LockOffset of LockTarget (height ignored), it locks in place.
 * After that the zone ignores it until it leaves the area, so unlocking it from Blueprint lets the player move it out.
 */
UCLASS(Blueprintable)
class ASALTOPROJECT_API AMovableLockZone : public AActor
{
	GENERATED_BODY()

public:
	AMovableLockZone();

	/** Distance from LockTarget at which the movable locks, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock Zone", meta = (ClampMin = 1.0, Units = "cm"))
	float LockOffset = 20.0f;

	/** If set, only this movable locks here. Empty = any movable. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Lock Zone")
	TObjectPtr<AMovableBase> RequiredMovable = nullptr;

	/** Draws the lock circle around LockTarget while a movable is inside. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock Zone|Debug")
	bool bDebugDraw = false;

	/** Fired when this zone locks a movable (open a door, play a sound...). */
	UPROPERTY(BlueprintAssignable, Category = "Lock Zone")
	FOnZoneMovableLocked OnMovableLocked;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnAreaEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	/** Detects movables. Size it in the editor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> LockArea;

	/** Where the movable should end up. Move it in the editor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> LockTarget;

private:
	/** Movables inside the area that can still lock here. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AMovableBase>> Candidates;

	/** Movables this zone locked; ignored until they leave the area. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AMovableBase>> LockedHere;
};