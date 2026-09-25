#include "StealthSightConeComponent.h"
#include "SceneManagement.h"
#include "PrimitiveSceneProxy.h"

class FStealthSightConeSceneProxy final : public FPrimitiveSceneProxy
{
public:
	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	explicit FStealthSightConeSceneProxy(const UStealthSightConeComponent* InComponent)
		: FPrimitiveSceneProxy(InComponent)
		, SightRadius(InComponent->SightRadius)
		, HalfAngleDegrees(FMath::Max(InComponent->PeripheralVisionAngleDegrees, 1.0f))
		, Color(InComponent->ConeColor)
	{
		bWillEverBeLit = false;
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
				DrawWireSphereCappedCone(PDI, FTransform(GetLocalToWorld()), SightRadius, HalfAngleDegrees, 24, 4, 8, Color, SDPG_World);
			}
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bDynamicRelevance = true;
		Result.bShadowRelevance = false;
		Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
		return Result;
	}

	virtual uint32 GetMemoryFootprint(void) const override { return sizeof(*this) + GetAllocatedSize(); }
	uint32 GetAllocatedSize(void) const { return FPrimitiveSceneProxy::GetAllocatedSize(); }

private:
	float SightRadius;
	float HalfAngleDegrees;
	FLinearColor Color;
};

UStealthSightConeComponent::UStealthSightConeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCastShadow(false);
}

FPrimitiveSceneProxy* UStealthSightConeComponent::CreateSceneProxy()
{
	return new FStealthSightConeSceneProxy(this);
}

FBoxSphereBounds UStealthSightConeComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds(FSphere(FVector::ZeroVector, SightRadius)).TransformBy(LocalToWorld);
}

void UStealthSightConeComponent::RefreshConeParameters(float NewSightRadius, float NewPeripheralVisionAngleDegrees)
{
	SightRadius = NewSightRadius;
	PeripheralVisionAngleDegrees = NewPeripheralVisionAngleDegrees;
	MarkRenderStateDirty();
}
