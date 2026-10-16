// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Interactables/EAllowedInteractor.h"
#include "InteractionComponent.generated.h"

class IInteractable;
class AInteractableBase;
class USphereComponent;
class UInputAction;

/**
 * Component that handles interaction detection and execution.
 * Interaction requests route through this component's Server RPCs since
 * clients own their character (and its components) but not world actors.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ASALTOPROJECT_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Where facing and traces start from when not using the character (for example a camera or an aim socket). */
	UPROPERTY(BlueprintReadWrite, Category = "Interaction")
	USceneComponent* TraceOrigin = nullptr;

	/** How far the facing line reaches, in cm. Used for the aim line and the debug draw. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = 50.0, ClampMax = 1000.0))
	float TraceDistance = 300.0f;

	/** Radius of the aim sphere trace, in cm. A bigger value makes aiming more forgiving. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = 1.0, ClampMax = 50.0))
	float TraceSphereRadius = 5.0f;

	/** Take facing from CustomTraceDirection instead of the character or trace origin forward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bUseCustomDirection = false;

	/** Facing direction used when bUseCustomDirection is on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (EditCondition = "bUseCustomDirection"))
	FVector CustomTraceDirection = FVector::ForwardVector;

	// --- Detection origin ---

	/** If true, the detection traces originate from the character; if false, from TraceOrigin (e.g. camera) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bTraceFromCharacter = true;

	/** Height above the character's origin where the detection trace starts (e.g. chest height) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (EditCondition = "bTraceFromCharacter"))
	float TraceHeight = 50.0f;

	// --- Scoring weights (choose freely; both terms are normalized to 0..1 before weighting) ---

	/** Weight of how directly the character faces the object */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Scoring", meta = (ClampMin = 0.0))
	float FacingWeight = 1.0f;

	/** Weight of how close the object is */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Scoring", meta = (ClampMin = 0.0))
	float DistanceWeight = 1.0f;

	/** Facing cone gate: objects whose facing dot is below this are ignored (-1 = 360, 0 = front 180, 0.7 ~ 45 cone) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Scoring", meta = (ClampMin = -1.0, ClampMax = 1.0))
	float MinFacingDot = 0.0f;

	/** Minimum score required before an object can become the focused interactable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Scoring")
	float MinScoreThreshold = 0.0f;

	// --- Debug ---

	/** Draws the interaction zone, scored candidates, facing trace and chosen target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Debug")
	bool bDebugDraw = false;

protected:
	/** The character this component sits on. Set in BeginPlay. */
	UPROPERTY()
	ACharacter* OwnerCharacter = nullptr;

	/** Sphere around the character. Actors that overlap it and implement IInteractable become candidates. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* InteractionZone;

	/** Radius of the interaction sphere, in cm. Doubles as the range that normalizes distance in the score. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = 50.0, ClampMax = 1000.0))
	float InteractionRadius = 250.0f;

	/** Seconds between checks while candidates are nearby but none is focused. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = 0.01, ClampMax = 0.5))
	float InteractionCheckInterval = 0.1f;

	/** Faster check interval used while an interactable is focused */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = 0.01, ClampMax = 0.5))
	float FocusedCheckInterval = 0.05f;

	/** If true, an object selected purely by proximity must have line of sight to be focused */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bRequireLineOfSight = true;

	/** Current repeat rate of the check timer (for adaptive interval switching) */
	float CurrentCheckRate = 0.0f;

	/** Collision channel the interaction sphere and the line-of-sight trace use to find interactables. */
	UPROPERTY(EditAnywhere, Category = "Interaction")
	TEnumAsByte<ECollisionChannel> InteractableChannel = ECC_GameTraceChannel2;

	/** Enhanced Input action for interaction. The owning character or controller binds it to DoInteract*. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* InteractAction;

	/** Candidates currently inside the interaction sphere. Scored each check to pick the focus. */
	TArray<TScriptInterface<IInteractable>> NearbyInteractables;

	/** The candidate that won focus in the last check (highest score with line of sight). */
	TScriptInterface<IInteractable> CurrentInteractable;

	/** The primitive on CurrentInteractable that gets the focus outline. */
	UPROPERTY()
	UPrimitiveComponent* CurrentHitComponent = nullptr;

	/** Repeating timer that runs CheckAimedInteractable. This component never uses Tick. */
	FTimerHandle InteractionCheckTimer;

	/** Set by SetFocusLocked. While true CheckAimedInteractable keeps the current focus. */
	bool bFocusLocked = false;

public:
	UInteractionComponent();

	/** Who this interactor is. Interactables use it to accept or refuse the interaction (see EAllowedInteractor). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	EInteractorRole InteractorRole = EInteractorRole::None;

	/** While locked the focus stays on the current object (e.g. while holding a movable), so release reaches it. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetFocusLocked(bool bLocked) { bFocusLocked = bLocked; }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsFocusLocked() const { return bFocusLocked; }

	/** Call on interact press. Runs BeginInteract on the focused object (server directly, client through the server RPC). */
	void DoInteractPressed();

	/** Call on interact release. Runs EndInteract on the focused object, which Hold interactions need. */
	void DoInteractReleased();

	/**
	 * Scores a candidate interactable. Higher scores win focus.
	 * Override in Blueprint to tune how facing and distance are weighted.
	 * @param Interactable  The candidate actor
	 * @param Distance      Distance from the detection origin to the candidate
	 * @param FacingDot     Dot product of facing direction and direction to candidate (-1..1)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Scoring")
	float ScoreInteractable(AActor* Interactable, float Distance, float FacingDot) const;
	virtual float ScoreInteractable_Implementation(AActor* Interactable, float Distance, float FacingDot) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Adds an overlapping interactable to the candidate list and starts the check timer on the first one. */
	UFUNCTION()
	void OnInteractionZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Removes a candidate that left the sphere and stops the timer once none remain. */
	UFUNCTION()
	void OnInteractionZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Timer body: scores every candidate, keeps the best one with line of sight, and updates the focus. */
	void CheckAimedInteractable();

	/** Changes the focused object: EndFocus on the previous one, BeginFocus on the new one. */
	void UpdateCurrentInteractable(TScriptInterface<IInteractable> NewInteractable, UPrimitiveComponent* HitComponent);

	/** Returns the world-space origin the detection traces/facing start from */
	FVector GetDetectionOrigin() const;

	/** Returns the facing direction used for scoring (character forward or trace origin forward) */
	FVector GetFacingDirection() const;

	/** Returns a representative primitive component to focus on the given actor */
	UPrimitiveComponent* ResolveFocusComponent(AActor* Actor, UPrimitiveComponent* AimedComponent) const;

	/** Line-of-sight check from the detection origin to the target actor */
	bool HasLineOfSightTo(AActor* Target) const;

	/** Switches the check timer between idle and focused rates when needed */
	void ApplyCheckInterval(bool bFocused);
};