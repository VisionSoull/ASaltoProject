// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractionFeedbackComponent.h"
#include "../Feedback/InteractionFeedbackProfile.h"
#include "../Feedback/InteractionHighlighter.h"
#include "../Feedback/InteractionWidget.h"
#include "Components/PrimitiveComponent.h"
#include "../Settings/InteractionSettings.h"

UInteractionFeedbackComponent::UInteractionFeedbackComponent()
{
	// No widget until ShowWidget is called
	WidgetClass = nullptr;

	// Screen space draws straight to the viewport: no render target per object
	Space = EWidgetSpace::Screen;
	bDrawAtDesiredSize = true;

	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetVisibility(false);

	// Only ticks while shown (the base class keeps the screen widget placed on tick)
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UInteractionFeedbackComponent::SetProfileOverride(UInteractionFeedbackProfile* NewProfile)
{
	ClearHighlight();
	ProfileOverride = NewProfile;
}

UInteractionFeedbackProfile* UInteractionFeedbackComponent::GetActiveProfile() const
{
	if (ProfileOverride)
	{
		return ProfileOverride;
	}
	return UInteractionSettings::GetInteractionSettings()->DefaultFeedbackProfile.LoadSynchronous();
}

UInteractionWidget* UInteractionFeedbackComponent::ShowWidget()
{
	const UInteractionFeedbackProfile* Profile = GetActiveProfile();
	if (!Profile || !Profile->WidgetClass)
	{
		HideWidget();
		return nullptr;
	}

	// First call, or the profile now asks for a different widget: (re)create it. Otherwise reuse.
	if (GetWidgetClass() != Profile->WidgetClass || !GetUserWidgetObject())
	{
		SetWidgetClass(Profile->WidgetClass);
		InitWidget();
	}

	SetRelativeLocation(Profile->WidgetOffset);
	SetVisibility(true);
	SetComponentTickEnabled(true);
	return GetInteractionWidget();
}

void UInteractionFeedbackComponent::HideWidget()
{
	if (!IsVisible())
	{
		return;
	}

	// Keep the widget object for next time; only take it off the screen
	SetVisibility(false);
	RemoveWidgetFromScreen();
	SetComponentTickEnabled(false);
}

UInteractionWidget* UInteractionFeedbackComponent::GetInteractionWidget() const
{
	return Cast<UInteractionWidget>(GetUserWidgetObject());
}

bool UInteractionFeedbackComponent::IsWidgetShown() const
{
	return IsVisible() && GetUserWidgetObject() != nullptr;
}

void UInteractionFeedbackComponent::ApplyHighlight(UPrimitiveComponent* Component)
{
	ClearHighlight();

	const UInteractionFeedbackProfile* Profile = GetActiveProfile();
	if (!Component || !Profile || !Profile->Highlighter)
	{
		return;
	}

	Profile->Highlighter->ApplyHighlight(Component);
	HighlightedComponent = Component;
	AppliedHighlighter = Profile->Highlighter;
}

void UInteractionFeedbackComponent::ClearHighlight()
{
	if (AppliedHighlighter && HighlightedComponent.IsValid())
	{
		AppliedHighlighter->ClearHighlight(HighlightedComponent.Get());
	}

	HighlightedComponent.Reset();
	AppliedHighlighter = nullptr;
}

void UInteractionFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Safety net in case the Blueprint did not clean up
	ClearHighlight();
	HideWidget();
	Super::EndPlay(EndPlayReason);
}