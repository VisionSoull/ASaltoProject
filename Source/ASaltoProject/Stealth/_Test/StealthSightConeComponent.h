#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "StealthSightConeComponent.generated.h"

/**
 * Editor-only wireframe visualization of an AI's sight volume: a sphere-capped cone, matching
 * exactly what UAISenseConfig_Sight actually tests (within SightRadius AND within the peripheral
 * vision half-angle). Renders via its own scene proxy, so it's visible in the editor viewport at
 * all times, no Play and no Gameplay Debugger required, and it updates live as you drag/rotate
 * the actor. Stripped from packaged builds since it's only ever created as editor-only data.
 */
UCLASS(ClassGroup = (Stealth), meta = (BlueprintSpawnableComponent))
class ASALTOPROJECT_API UStealthSightConeComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UStealthSightConeComponent();

	UPROPERTY(EditAnywhere, Category = "Stealth|Debug")
	float SightRadius = 1500.0f;

	UPROPERTY(EditAnywhere, Category = "Stealth|Debug")
	float PeripheralVisionAngleDegrees = 90.0f;

	UPROPERTY(EditAnywhere, Category = "Stealth|Debug")
	FLinearColor ConeColor = FLinearColor(1.0f, 0.15f, 0.15f, 1.0f);

	// Call after changing SightRadius/PeripheralVisionAngleDegrees in code so the cone redraws to match.
	void RefreshConeParameters(float NewSightRadius, float NewPeripheralVisionAngleDegrees);

	//~ Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ End UPrimitiveComponent Interface
};
