// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FranciscoTypes.h"
#include "Components/SphereComponent.h"
#include "ProximityComponent.generated.h"

class AFranciscoCharacter;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ASALTOPROJECT_API UProximityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UProximityComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// ---------------------------------------------------------------------
	// Zone radii (also used as the radius of each overlap sphere)
	// Panic has no sphere: it is simply "outside DangerDistance".
	// ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Thresholds", meta = (AllowPrivateAccess = "true"))
	float ComfortDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Thresholds", meta = (AllowPrivateAccess = "true"))
	float NormalDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Thresholds", meta = (AllowPrivateAccess = "true"))
	float WarningDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Thresholds", meta = (AllowPrivateAccess = "true"))
	float DangerDistance;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Thresholds", meta = (AllowPrivateAccess = "true"))
	float ReconnectDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Thresholds", meta = (AllowPrivateAccess = "true"))
	float MaxHandHoldDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance", meta = (AllowPrivateAccess = "true"))
	float UpdateInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	bool bDebugDrawDistance;

	// Current state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status", meta = (AllowPrivateAccess = "true"))
	float CurrentDistanceToPlayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status", meta = (AllowPrivateAccess = "true"))
	EProximityZone CurrentZone;

	// Internal
	float UpdateTimer;
	float LastDistance;
	EProximityZone LastZone;

	UFUNCTION()
	void OnStateChanged(EFranciscoState OldState, EFranciscoState NewState);

	UPROPERTY()
	AFranciscoCharacter* OwnerCharacter;

	// ---------------------------------------------------------------------
	// Overlap spheres
	// ---------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proximity Spheres", meta = (AllowPrivateAccess = "true"))
	USphereComponent* ComfortSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proximity Spheres", meta = (AllowPrivateAccess = "true"))
	USphereComponent* NormalSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proximity Spheres", meta = (AllowPrivateAccess = "true"))
	USphereComponent* WarningSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proximity Spheres", meta = (AllowPrivateAccess = "true"))
	USphereComponent* DangerSphere;

	bool bPlayerInComfort;
	bool bPlayerInNormal;
	bool bPlayerInWarning;
	bool bPlayerInDanger;

	bool bInHandHoldRange;

	bool bInitialSyncDone;

	bool bUpdatingSpheres;

	bool IsRelevantOverlap(AActor* OtherActor, UPrimitiveComponent* OtherComp) const;
	void UpdateZoneFromOverlaps();
	void UpdateActiveSpheres();
	void DrawDebugInfo() const;

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnProximityChanged OnProximityChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnZoneChanged OnZoneChanged;

	UFUNCTION(BlueprintCallable, Category = "Proximity")
	float GetDistanceToPlayer() const { return CurrentDistanceToPlayer; }

	UFUNCTION(BlueprintCallable, Category = "Proximity")
	EProximityZone GetCurrentZone() const { return CurrentZone; }

	UFUNCTION(BlueprintCallable, Category = "Proximity")
	bool IsInComfortZone() const { return CurrentZone == EProximityZone::Comfort; }

	UFUNCTION(BlueprintCallable, Category = "Proximity")
	bool ShouldPanic() const;

	UFUNCTION(BlueprintCallable, Category = "Proximity")
	bool IsPlayerInReconnectRange() const { return bPlayerInNormal || bPlayerInComfort; }

	UFUNCTION(BlueprintCallable, Category = "Proximity")
	bool IsInHandHoldRange() const { return bInHandHoldRange; }

	// Sphere overlap callbacks
	UFUNCTION()
	void OnComfortSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnComfortSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnNormalSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnNormalSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnWarningSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnWarningSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnDangerSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnDangerSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};