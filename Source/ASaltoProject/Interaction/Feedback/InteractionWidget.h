// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionWidget.generated.h"

/**
 * Base for interaction prompt widgets. UInteractionFeedbackComponent calls these events;
 * the Widget Blueprint decides the look (idle dot, expand animation, prompt text).
 */
UCLASS(Abstract)
class ASALTOPROJECT_API UInteractionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The object came into the player's interaction range. Show the idle state, e.g. a small dot. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnEnterRange();

	/** The object left the range. The widget is hidden right after this. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnExitRange();

	/** The object became the focused one. Expand or animate, and show PromptText. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnFocused(const FText& PromptText);

	/** The object lost focus but is still in range. Go back to the idle state. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Interaction|Feedback")
	void OnUnfocused();
};