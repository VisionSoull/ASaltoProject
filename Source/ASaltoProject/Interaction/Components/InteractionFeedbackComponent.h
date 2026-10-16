// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "InteractionFeedbackComponent.generated.h"

class UInteractionFeedbackProfile;
class UInteractionHighlighter;
class UInteractionWidget;
class UPrimitiveComponent;

/**
 * Feedback toolbox for an interactable: functions to show a screen-space prompt widget
 * and to highlight a mesh, both configured by a feedback profile. Shows nothing on its own:
 * the interactable's Blueprint decides when to call them.
 * Starts with no widget class, so it costs nothing until ShowWidget is first called;
 * after that the widget is hidden and reused, never destroyed and rebuilt.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ASALTOPROJECT_API UInteractionFeedbackComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UInteractionFeedbackComponent();

	/** Profile for this object. Empty = Default Feedback Profile in Project Settings > Interaction Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Feedback")
	TObjectPtr<UInteractionFeedbackProfile> ProfileOverride;

	/** Switches this object's profile at runtime. Clears the current highlight; the next ShowWidget uses the new widget. */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Feedback")
	void SetProfileOverride(UInteractionFeedbackProfile* NewProfile);

	/** ProfileOverride if set, otherwise the project default. Can be null. */
	UFUNCTION(BlueprintPure, Category = "Interaction|Feedback")
	UInteractionFeedbackProfile* GetActiveProfile() const;

	/** Shows the profile's widget, creating it the first time. Returns it, or null if the profile has no widget. */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Feedback")
	UInteractionWidget* ShowWidget();

	/** Hides the widget and keeps it for the next ShowWidget. */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Feedback")
	void HideWidget();

	/** The widget if it was already created (shown or hidden), otherwise null. Never creates it. */
	UFUNCTION(BlueprintPure, Category = "Interaction|Feedback")
	UInteractionWidget* GetInteractionWidget() const;

	/** True while the widget is on screen. */
	UFUNCTION(BlueprintPure, Category = "Interaction|Feedback")
	bool IsWidgetShown() const;

	/** Highlights Component with the profile's highlighter. Replaces any previous highlight from this component. */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Feedback")
	void ApplyHighlight(UPrimitiveComponent* Component);

	/** Removes the highlight applied by ApplyHighlight, if any. */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Feedback")
	void ClearHighlight();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Component highlighted by the last ApplyHighlight. */
	TWeakObjectPtr<UPrimitiveComponent> HighlightedComponent;

	/** Highlighter used by the last ApplyHighlight, so ClearHighlight undoes it even if the profile changed. */
	UPROPERTY(Transient)
	TObjectPtr<UInteractionHighlighter> AppliedHighlighter;
};