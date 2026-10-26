#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGHipRoof.generated.h"

/**
 * Hip roof over a closed footprint.
 * Input: the footprint corners at the top of the walls, in order.
 * Output: one point set per mesh kind, ready for Static Mesh Spawners.
 */
UCLASS(BlueprintType, ClassGroup = (Procedural))
class ASALTOPROJECT_API UPCGHipRoofSettings : public UPCGSettings
{
    GENERATED_BODY()

public:
#if WITH_EDITOR
    virtual FName GetDefaultNodeName() const override { return FName(TEXT("HipRoof")); }
    virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGHipRoof", "NodeTitle", "Hip Roof"); }
    virtual FText GetNodeTooltipText() const override { return NSLOCTEXT("PCGHipRoof", "NodeTooltip", "Hip roof over the footprint corners."); }
    virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spatial; }
#endif

protected:
    virtual TArray<FPCGPinProperties> InputPinProperties() const override;
    virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
    virtual FPCGElementPtr CreateElement() const override;

public:
    /** Rise per unit of run. 0.625 is the kit roof pitch. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Roof", meta = (PCG_Overridable))
    double Pitch = 0.625;

    /** How far the eaves stick out past the walls. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Roof", meta = (PCG_Overridable))
    double Overhang = 30.0;

    /** Height of the roof above the wall top, measured at the wall line. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Roof", meta = (PCG_Overridable))
    double EaveLift = 20.0;

    /** Safety limit on inset steps. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Roof", meta = (ClampMin = "1", PCG_Overridable))
    int32 MaxLevels = 32;

    /** Spacing of tile columns along the eave. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tiles", meta = (PCG_Overridable))
    double TileWidth = 14.3;

    /** Length of the tile mesh along the slope. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tiles", meta = (PCG_Overridable))
    double TileLength = 30.0;

    /** Spacing of tile courses along the slope; less than TileLength so tiles overlap. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tiles", meta = (PCG_Overridable))
    double TileCourse = 26.0;

    /** Length of the cap mesh. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Caps", meta = (PCG_Overridable))
    double CapLength = 236.0;

    /** Caps repeat at this length, stretched to fill the row. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Caps", meta = (PCG_Overridable))
    double CapModule = 177.0;

    /** Lift of the caps so they sit on the tiles. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Caps", meta = (PCG_Overridable))
    double CapLift = 6.0;
};

class FPCGHipRoofElement : public IPCGElement
{
protected:
    virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
