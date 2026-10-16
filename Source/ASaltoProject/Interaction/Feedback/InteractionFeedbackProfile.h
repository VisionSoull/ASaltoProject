// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InteractionHighlighter.h"
#include "InteractionWidget.h"
#include "InteractionFeedbackProfile.generated.h"

/**
 * Look of an interactable: how it is highlighted when focused and which widget shows while in range.
 * One profile is set as the project default in Interaction Settings; objects can pick another one.
 */
UCLASS(BlueprintType)
class ASALTOPROJECT_API UInteractionFeedbackProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** How the focused object is highlighted (Overlay Material or Custom Depth Stencil). Empty = no outline. */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Outline")
	TObjectPtr<UInteractionHighlighter> Highlighter;

	/** Widget shown while the object is in range (idle) and evolved while focused. Empty = no widget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Widget")
	TSubclassOf<UInteractionWidget> WidgetClass;

	/** Where the widget sits relative to the object's root, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Widget")
	FVector WidgetOffset = FVector(0.f, 0.f, 100.f);

	/** Text handed to the widget when focused, e.g. "Pull". Can stay empty for icon-only prompts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Widget")
	FText PromptText;
};