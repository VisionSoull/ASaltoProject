// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InteractionHighlighter.generated.h"

class UPrimitiveComponent;
class UMaterialInterface;

/**
 * How a focused interactable is highlighted. Chosen per feedback profile.
 * Subclass in C++ or Blueprint for new looks. ClearHighlight must undo ApplyHighlight.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced, CollapseCategories)
class ASALTOPROJECT_API UInteractionHighlighter : public UObject
{
	GENERATED_BODY()

public:
	/** Turns the highlight on for Component. Does nothing for null. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Feedback")
	void ApplyHighlight(UPrimitiveComponent* Component);

	/** Turns the highlight off for Component. Does nothing for null. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Feedback")
	void ClearHighlight(UPrimitiveComponent* Component);

protected:
	virtual void ApplyHighlight_Implementation(UPrimitiveComponent* Component);
	virtual void ClearHighlight_Implementation(UPrimitiveComponent* Component);
};

/** Draws OverlayMaterial on top of the focused mesh. Needs no level setup. */
UCLASS(meta = (DisplayName = "Overlay Material"))
class ASALTOPROJECT_API UOverlayHighlighter : public UInteractionHighlighter
{
	GENERATED_BODY()

public:
	/** Material drawn over the mesh while focused, e.g. a translucent fresnel rim. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	TObjectPtr<UMaterialInterface> OverlayMaterial;

protected:
	virtual void ApplyHighlight_Implementation(UPrimitiveComponent* Component) override;
	virtual void ClearHighlight_Implementation(UPrimitiveComponent* Component) override;
};

/**
 * Writes StencilValue to custom depth so a post-process outline material can draw it.
 * Needs Custom Depth-Stencil Pass = Enabled with Stencil (r.CustomDepth=3) and the outline post-process in the level.
 */
UCLASS(meta = (DisplayName = "Custom Depth Stencil"))
class ASALTOPROJECT_API UStencilHighlighter : public UInteractionHighlighter
{
	GENERATED_BODY()

public:
	/** Value the post-process reads. Use different values for different outline colors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight", meta = (ClampMin = 1, ClampMax = 255))
	int32 StencilValue = 1;

protected:
	virtual void ApplyHighlight_Implementation(UPrimitiveComponent* Component) override;
	virtual void ClearHighlight_Implementation(UPrimitiveComponent* Component) override;
};
