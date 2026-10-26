#include "PCGHipRoof.h"

#include "Algo/Reverse.h"
#include "PCGContext.h"
#include "PCGPin.h"
#include "Data/PCGBasePointData.h"
#include "Helpers/PCGHelpers.h"

// ==================================================================================
// How the roof is built
// - The footprint is pushed out by Overhang to get the eave outline.
// - Each level moves every edge inward (and up the slope) until the first edge
//   shrinks to nothing, and covers the band it swept with planes, corner
//   triangles, hip caps and tiles. The surviving edges become the next outline.
// - When an outline has collapsed to a line it is the ridge.
// This is a straight skeleton with edge events only: a wall that would split
// a roof face in two (wide T or U plans) is not handled.
//
// Meshes the outputs are made for (all in ModularKits):
// - Planes, Hip/Valley Corners: SM_BG_RoofPlane/Hip/Valley, 100 units, sloping up toward -Y.
// - Caps: SM_RoofEdgeBig_01, runs along +Y.
// - Tiles: SM_BG_RoofTile, runs up the slope along +X from its lower end.
// ==================================================================================

namespace PCGHipRoof
{
    const FName FootprintLabel = TEXT("Footprint");
    const FName PlanesLabel = TEXT("Planes");
    const FName HipsLabel = TEXT("Hip Corners");
    const FName ValleysLabel = TEXT("Valley Corners");
    const FName CapsLabel = TEXT("Caps");
    const FName TilesLabel = TEXT("Tiles");

    // Outlines with less area than this (cm^2) have collapsed to a ridge line.
    constexpr double FlatArea = 1000.0;

    struct FEdge
    {
        FVector S, E;   // start, end
        FVector D, N;   // direction, inward normal
        double Len = 0.0;
        double Cs = 0.0; // how far the start corner slides along D per unit of inset (> 0 hip, < 0 valley)
        double Ce = 0.0; // same for the end corner, against D
    };

    // Inset slide rate at the corner between two edge directions: tan of half the turn.
    double CornerRate(const FVector& DIn, const FVector& DOut)
    {
        const FVector NIn = FVector::CrossProduct(DIn, FVector::UpVector);
        const FVector NOut = FVector::CrossProduct(DOut, FVector::UpVector);
        const double Den = FVector::DotProduct(DOut, NIn);
        return FMath::Abs(Den) < UE_SMALL_NUMBER ? 0.0 : (1.0 - FVector::DotProduct(NOut, NIn)) / Den;
    }

    // Edges of a closed outline whose interior is on the right of each edge.
    TArray<FEdge> MakeEdges(const TArray<FVector>& Poly)
    {
        const int32 Num = Poly.Num();
        TArray<FEdge> Edges;
        Edges.SetNum(Num);
        for (int32 i = 0; i < Num; ++i)
        {
            FEdge& Edge = Edges[i];
            Edge.S = Poly[i];
            Edge.E = Poly[(i + 1) % Num];
            Edge.Len = FVector::Dist(Edge.S, Edge.E);
            Edge.D = (Edge.E - Edge.S).GetSafeNormal();
            Edge.N = FVector::CrossProduct(Edge.D, FVector::UpVector);
        }
        for (int32 i = 0; i < Num; ++i)
        {
            const double Rate = CornerRate(Edges[(i + Num - 1) % Num].D, Edges[i].D);
            Edges[i].Cs = Rate;
            Edges[(i + Num - 1) % Num].Ce = Rate;
        }
        return Edges;
    }

    double SignedArea(const TArray<FVector>& Poly)
    {
        double Twice = 0.0;
        for (int32 i = 0; i < Poly.Num(); ++i)
        {
            const FVector& A = Poly[i];
            const FVector& B = Poly[(i + 1) % Poly.Num()];
            Twice += A.X * B.Y - B.X * A.Y;
        }
        return Twice * 0.5;
    }

    class FBuilder
    {
    public:
        explicit FBuilder(const UPCGHipRoofSettings& InSettings)
            : Settings(InSettings)
            , Slope(FMath::Sqrt(1.0 + InSettings.Pitch * InSettings.Pitch))
        {
        }

        TArray<FTransform> Planes, Hips, Valleys, Caps, Tiles;

        void Build(TArray<FVector> Footprint)
        {
            if (Footprint.Num() < 3)
            {
                return;
            }
            if (SignedArea(Footprint) > 0.0)
            {
                Algo::Reverse(Footprint);
            }

            // eave outline: corners pushed out along their miters, dropped to keep the slope through the wall line
            const TArray<FEdge> Walls = MakeEdges(Footprint);
            const double EaveDrop = Settings.EaveLift - Settings.Pitch * Settings.Overhang;
            TArray<FVector> Outline;
            for (const FEdge& Wall : Walls)
            {
                Outline.Add(Wall.S - (Wall.N + Wall.D * Wall.Cs) * Settings.Overhang + FVector(0.0, 0.0, EaveDrop));
            }
            EaveZ = Outline[0].Z;

            for (int32 Level = 0; Level < Settings.MaxLevels && Outline.Num() >= 2; ++Level)
            {
                if (FMath::Abs(SignedArea(Outline)) < FlatArea)
                {
                    AddRidge(MakeEdges(Outline));
                    break;
                }
                Outline = AddLevel(MakeEdges(Outline));
            }
        }

    private:
        const UPCGHipRoofSettings& Settings;
        const double Slope; // slope length per unit of run
        double EaveZ = 0.0;

        // One inset step. Returns the next outline.
        TArray<FVector> AddLevel(const TArray<FEdge>& Edges)
        {
            double Run = TNumericLimits<double>::Max();
            for (const FEdge& Edge : Edges)
            {
                Run = FMath::Min(Run, Edge.Len / FMath::Max(Edge.Cs + Edge.Ce, 1e-4));
            }
            const FVector Rise(0.0, 0.0, Settings.Pitch * Run);

            TArray<FVector> Next;
            for (const FEdge& Edge : Edges)
            {
                const double Rs = Run * Edge.Cs;
                const double Re = Run * Edge.Ce;
                const FQuat Rot = FRotationMatrix::MakeFromXZ(Edge.D, FVector::UpVector).ToQuat();
                const double RunScale = Run * 0.01;

                // the band's rectangle between the two corner triangles
                const double RectLen = Edge.Len - FMath::Max(Rs, 0.0) - FMath::Max(Re, 0.0);
                if (RectLen > 1.0)
                {
                    Planes.Emplace(Rot, Edge.S + Edge.D * FMath::Max(Rs, 0.0), FVector(RectLen * 0.01, RunScale, RunScale));
                }

                // corner triangles; the end one is mirrored
                AddCorner(Rs, FTransform(Rot, Edge.S, FVector(Rs * 0.01, RunScale, RunScale)));
                AddCorner(Re, FTransform(Rot, Edge.E, FVector(-Re * 0.01, RunScale, RunScale)));

                if (Edge.Cs > 0.001)
                {
                    AddCapRow(Edge.S, (Edge.N + Edge.D * Edge.Cs) * Run + Rise);
                }

                AddTiles(Edge, Run);

                if (Edge.Len - Run * (Edge.Cs + Edge.Ce) > 0.5)
                {
                    Next.Add(Edge.S + Edge.N * Run + Edge.D * Rs + Rise);
                }
            }
            return Next;
        }

        void AddCorner(double Inset, const FTransform& Transform)
        {
            if (Inset > 0.5)
            {
                Hips.Add(Transform);
            }
            else if (Inset < -0.5)
            {
                Valleys.Add(Transform);
            }
        }

        // A collapsed outline runs there and back along the ridge; cap one direction only.
        void AddRidge(const TArray<FEdge>& Edges)
        {
            for (const FEdge& Edge : Edges)
            {
                if (Edge.D.X > 0.001 || (FMath::Abs(Edge.D.X) <= 0.001 && Edge.D.Y > 0.0))
                {
                    AddCapRow(Edge.S, Edge.E - Edge.S);
                }
            }
        }

        // Cap pieces from Start along Vec, stretched to fill whole modules; short rows get one squashed piece.
        void AddCapRow(const FVector& Start, const FVector& Vec)
        {
            const double Len = Vec.Size();
            if (Len <= 5.0)
            {
                return;
            }
            const FVector Dir = Vec / Len;
            const FQuat Rot = FRotationMatrix::MakeFromYZ(Dir, FVector::UpVector).ToQuat();
            const FVector Lift(0.0, 0.0, Settings.CapLift);
            const int32 Count = Len >= Settings.CapModule ? FMath::FloorToInt32(Len / Settings.CapModule) : 1;
            const double Module = Len / Count;
            for (int32 i = 0; i < Count; ++i)
            {
                Caps.Emplace(Rot, Start + Dir * (Module * i) + Lift, FVector(1.0, Module / Settings.CapLength, 1.0));
            }
        }

        // Tile columns on a world grid along the edge, cut at the hip and valley lines.
        // Courses are counted in slope distance from the eave so they line up across levels.
        void AddTiles(const FEdge& Edge, double Run)
        {
            const double W = Settings.TileWidth;
            const double C = Settings.TileCourse;
            const double UStart = FMath::Min(Run * Edge.Cs, 0.0);              // valleys reach past the edge ends
            const double UEnd = Edge.Len - FMath::Min(Run * Edge.Ce, 0.0);
            const double Proj = FVector::DotProduct(Edge.S, Edge.D);
            const double UFirst = FMath::CeilToDouble((Proj + UStart) / W) * W - Proj;
            if (UEnd < UFirst)
            {
                return;
            }

            const FVector UpSlope = (Edge.N + FVector::UpVector * Settings.Pitch).GetSafeNormal();
            const FQuat Rot = FRotationMatrix::MakeFromXY(UpSlope, Edge.D).ToQuat();
            const double BandDist = (Edge.S.Z - EaveZ) * Slope / Settings.Pitch;
            const double HipRateS = FMath::Max(Edge.Cs, 1e-3), HipRateE = FMath::Max(Edge.Ce, 1e-3);
            const double ValRateS = FMath::Min(Edge.Cs, -1e-3), ValRateE = FMath::Min(Edge.Ce, -1e-3);

            const int32 Columns = FMath::FloorToInt32((UEnd - UFirst) / W) + 1;
            for (int32 Col = 0; Col < Columns; ++Col)
            {
                const double U = UFirst + W * Col;
                const double TopRun = FMath::Min3(Run, U / HipRateS, (Edge.Len - U) / HipRateE);
                const double BotRun = FMath::Max3(0.0, U / ValRateS, (Edge.Len - U) / ValRateE);
                const double Bottom = BandDist + BotRun * Slope;
                const double Top = BandDist + TopRun * Slope;
                if (Top - Bottom <= 1.0)
                {
                    continue;
                }

                // point on the column at a slope distance from the eave
                const FVector Origin = Edge.S + Edge.D * U - UpSlope * BandDist;
                auto AddTile = [&](double At, double Length)
                {
                    Tiles.Emplace(Rot, Origin + UpSlope * At, FVector(Length / Settings.TileLength, 1.0, 1.0));
                };

                const double FirstCourse = FMath::CeilToDouble(Bottom / C) * C;
                const int32 Full = FMath::Max(FMath::FloorToInt32((Top - FirstCourse) / C), 0);
                for (int32 i = 0; i < Full; ++i)
                {
                    AddTile(FirstCourse + C * i, Settings.TileLength);
                }
                // cut to the hip, ridge or band top
                const double TopStart = FirstCourse + C * Full;
                if (Top - TopStart > 1.0)
                {
                    AddTile(TopStart, Top - TopStart);
                }
                // from a valley or the band start up to the first course, tucked under it
                const double BottomLen = FMath::Min(FirstCourse, Top) - Bottom;
                if (BottomLen > 1.0)
                {
                    AddTile(Bottom, BottomLen + Settings.TileLength - C);
                }
            }
        }
    };
}

TArray<FPCGPinProperties> UPCGHipRoofSettings::InputPinProperties() const
{
    TArray<FPCGPinProperties> Properties;
    Properties.Emplace(PCGHipRoof::FootprintLabel, EPCGDataType::Point);
    return Properties;
}

TArray<FPCGPinProperties> UPCGHipRoofSettings::OutputPinProperties() const
{
    TArray<FPCGPinProperties> Properties;
    for (const FName& Label : { PCGHipRoof::PlanesLabel, PCGHipRoof::HipsLabel, PCGHipRoof::ValleysLabel, PCGHipRoof::CapsLabel, PCGHipRoof::TilesLabel })
    {
        Properties.Emplace(Label, EPCGDataType::Point);
    }
    return Properties;
}

FPCGElementPtr UPCGHipRoofSettings::CreateElement() const
{
    return MakeShared<FPCGHipRoofElement>();
}

bool FPCGHipRoofElement::ExecuteInternal(FPCGContext* Context) const
{
    // ==================================================================================
    // One roof per input point set
    // ==================================================================================
    TRACE_CPUPROFILER_EVENT_SCOPE(FPCGHipRoofElement::Execute);
    check(Context);

    const UPCGHipRoofSettings* Settings = Context->GetInputSettings<UPCGHipRoofSettings>();
    check(Settings);

    auto Emit = [Context](const FName& Pin, const TArray<FTransform>& Transforms)
    {
        UPCGBasePointData* Points = FPCGContext::NewPointData_AnyThread(Context);
        Points->SetNumPoints(Transforms.Num());
        Points->AllocateProperties(EPCGPointNativeProperties::Transform | EPCGPointNativeProperties::Seed);
        TPCGValueRange<FTransform> TransformRange = Points->GetTransformValueRange(/*bAllocate=*/false);
        TPCGValueRange<int32> SeedRange = Points->GetSeedValueRange(/*bAllocate=*/false);
        for (int32 i = 0; i < Transforms.Num(); ++i)
        {
            TransformRange[i] = Transforms[i];
            SeedRange[i] = PCGHelpers::ComputeSeedFromPosition(Transforms[i].GetLocation());
        }
        FPCGTaggedData& Output = Context->OutputData.TaggedData.Emplace_GetRef();
        Output.Data = Points;
        Output.Pin = Pin;
    };

    for (const FPCGTaggedData& Input : Context->InputData.GetInputsByPin(PCGHipRoof::FootprintLabel))
    {
        const UPCGBasePointData* InPoints = Cast<UPCGBasePointData>(Input.Data);
        if (!InPoints)
        {
            continue;
        }

        TArray<FVector> Footprint;
        const TConstPCGValueRange<FTransform> InTransforms = InPoints->GetConstTransformValueRange();
        for (const FTransform& Transform : InTransforms)
        {
            Footprint.Add(Transform.GetLocation());
        }

        PCGHipRoof::FBuilder Builder(*Settings);
        Builder.Build(MoveTemp(Footprint));
        Emit(PCGHipRoof::PlanesLabel, Builder.Planes);
        Emit(PCGHipRoof::HipsLabel, Builder.Hips);
        Emit(PCGHipRoof::ValleysLabel, Builder.Valleys);
        Emit(PCGHipRoof::CapsLabel, Builder.Caps);
        Emit(PCGHipRoof::TilesLabel, Builder.Tiles);
    }

    return true;
}
