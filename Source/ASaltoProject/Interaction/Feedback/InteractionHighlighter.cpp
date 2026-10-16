// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractionHighlighter.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"

void UInteractionHighlighter::ApplyHighlight_Implementation(UPrimitiveComponent* Component)
{
	// Base does nothing; subclasses decide the look
}

void UInteractionHighlighter::ClearHighlight_Implementation(UPrimitiveComponent* Component)
{
}

void UOverlayHighlighter::ApplyHighlight_Implementation(UPrimitiveComponent* Component)
{
	if (UMeshComponent* Mesh = Cast<UMeshComponent>(Component))
	{
		Mesh->SetOverlayMaterial(OverlayMaterial);
	}
}

void UOverlayHighlighter::ClearHighlight_Implementation(UPrimitiveComponent* Component)
{
	if (UMeshComponent* Mesh = Cast<UMeshComponent>(Component))
	{
		Mesh->SetOverlayMaterial(nullptr);
	}
}

void UStencilHighlighter::ApplyHighlight_Implementation(UPrimitiveComponent* Component)
{
	if (Component)
	{
		Component->SetCustomDepthStencilValue(StencilValue);
		Component->SetRenderCustomDepth(true);
	}
}

void UStencilHighlighter::ClearHighlight_Implementation(UPrimitiveComponent* Component)
{
	if (Component)
	{
		Component->SetRenderCustomDepth(false);
	}
}