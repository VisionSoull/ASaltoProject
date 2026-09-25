#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "StealthComponent.generated.h"

/** Whether this actor's owner is currently getting away with it or not. */
UENUM(BlueprintType)
enum class EStealthState : uint8
{
	/** Not currently detected (or detection was Reset since the last time it was). */
	Hidden,
	/** Detected at least once since the last Reset. Stays true until ResetDetection() is called. */
	Detected
};

/** Everything about a single detection: who caused it, why, and where it happened. */
USTRUCT(BlueprintType)
struct FStealthDetectionEvent
{
	GENERATED_BODY()

	/** The actor that detected the player. Can be null for a scripted/story-driven detection
	 * with no specific instigator (e.g. a forced story beat). */
	UPROPERTY(BlueprintReadOnly, Category = "Stealth")
	TObjectPtr<AActor> Instigator = nullptr;

	/** What kind of detection this was (e.g. Detection.Sight, Detection.Sound, Detection.Scripted).
	 * Data-driven so designers can add new detection types from the tag editor without a C++ change. */
	UPROPERTY(BlueprintReadOnly, Category = "Stealth")
	FGameplayTag Reason;

	/** World location of the detection, useful for orienting a caught reaction/camera. */
	UPROPERTY(BlueprintReadOnly, Category = "Stealth")
	FVector Location = FVector::ZeroVector;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerDetected, const FStealthDetectionEvent&, DetectionEvent);

/**
 * Reports when the owning actor has been detected, for any reason (sight, sound, scripted, etc).
 * Deliberately does not decide what happens as a consequence, that is left entirely to whatever
 * binds to OnPlayerDetected (GameMode, UI, a cutscene trigger). This component only answers
 * "was I seen, by what, where".
 *
 * Any AI controller, trigger volume, or scripted Blueprint event can report a detection by calling
 * ReportDetection(), regardless of how it senses the player (AIPerceptionComponent, PawnSensing,
 * a scripted trigger with no sensing at all). Anything like a delayed/suspicion-meter style
 * detection is expected to live on the sensing side (whatever decides "have I seen them long
 * enough to be sure"), this component only needs to hear about it once that decision is made.
 */
UCLASS(ClassGroup = (Stealth), meta = (BlueprintSpawnableComponent))
class ASALTOPROJECT_API UStealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStealthComponent();

	/** Fired exactly once per Hidden -> Detected transition. Bind GameMode/UI/consequence logic
	 * here - this component never decides what happens next, that's entirely up to whatever's
	 * listening (game over, alarm, a cutscene, anything). */
	UPROPERTY(BlueprintAssignable, Category = "Stealth")
	FOnPlayerDetected OnPlayerDetected;

	/**
	 * Call this the moment it's certain the player has been detected. Safe to call repeatedly -
	 * only the first call after a Hidden -> Detected transition actually fires OnPlayerDetected,
	 * every call after that is a useless until ResetDetection() is called.
	 * @param Instigator	The actor that spotted the player (an AI controller/pawn, a trigger volume,
	 *						anything). Can be left null for a scripted detection with no specific source.
	 * @param Reason		Tag describing why this counts as a detection (e.g. Detection.Sight). Leaving
	 *						it unset is fine, it just means nothing downstream can distinguish the cause.
	 * @param Location		World location the detection happened at, for orienting a caught reaction/camera.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void ReportDetection(AActor* Instigator, FGameplayTag Reason, FVector Location);

	/** Returns to Hidden so detection can be reported again (e.g. after a checkpoint reload/retry).
	 * Does not broadcast anything by itself - it just clears the latch. */
	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void ResetDetection();

	/**
	 * Suspends or resumes detection entirely, e.g. during cutscenes or scripted safe-passage moments.
	 * @param bEnabled	False makes ReportDetection() calls silently do nothing until re-enabled.
	 *					Does not change the current Hidden/Detected state either way.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void SetDetectionEnabled(bool bEnabled);

	/** True once this actor has been detected and hasn't been Reset since. Use this for a Branch
	 * node when you just need a yes/no check instead of binding to OnPlayerDetected. */
	UFUNCTION(BlueprintPure, Category = "Stealth")
	bool IsDetected() const { return CurrentState == EStealthState::Detected; }

	/** Details of the most recent detection (who/why/where). Keeps returning the last detection
	 * even after ResetDetection() - check IsDetected() first if you need to know whether this is
	 * still the *current* state or just history. */
	UFUNCTION(BlueprintPure, Category = "Stealth")
	FStealthDetectionEvent GetLastDetectionEvent() const { return LastDetectionEvent; }

protected:
	virtual void BeginPlay() override;

private:
	/** Master on/off switch. While false, ReportDetection() is ignored entirely - use this for
	 * cutscenes/safe-passage moments instead of removing the component. */
	UPROPERTY(EditAnywhere, Category = "Stealth")
	bool bDetectionEnabled = true;

	/** Logs every ReportDetection()/ResetDetection() call (and why one was ignored) to the Output
	 * Log. Useful while the AI side is still being built, safe to leave off otherwise. */
	UPROPERTY(EditAnywhere, Category = "Stealth|Debug")
	bool bDrawDebug = false;

	/** Makes this actor detectable via AI Sight (line-of-sight/vision). Turn off if it should
	 * never be visually detected (e.g. it's only ever caught by sound). */
	UPROPERTY(EditAnywhere, Category = "Stealth|Senses")
	bool bRegisterSight = true;

	/** Makes this actor detectable via AI Hearing. Registering alone doesn't generate any noise -
	 * something still needs to call MakeNoise()/UAISense_Hearing::ReportNoiseEvent() whenever this
	 * actor does something sound-worthy (footsteps, etc), that's not this component's job. */
	UPROPERTY(EditAnywhere, Category = "Stealth|Senses")
	bool bRegisterHearing = false;

	/** Makes this actor detectable via AI Damage (UAISense_Damage). Registering alone doesn't
	 * report anything - something still needs to call UAISense_Damage::ReportDamageEvent() when
	 * this actor takes damage, that's not this component's job. */
	UPROPERTY(EditAnywhere, Category = "Stealth|Senses")
	bool bRegisterDamage = false;

	/** Makes this actor detectable via AI Touch (UAISense_Touch - physical contact/overlap based). */
	UPROPERTY(EditAnywhere, Category = "Stealth|Senses")
	bool bRegisterTouch = false;

	/** Current Hidden/Detected state. Shown here so you can watch it live in the Details panel
	 * during PIE - read it via IsDetected() from code/Blueprint rather than this property directly. */
	UPROPERTY(VisibleAnywhere, Category = "Stealth")
	EStealthState CurrentState = EStealthState::Hidden;

	/** Mirrors GetLastDetectionEvent(), shown here purely so it's inspectable live during PIE. */
	UPROPERTY(VisibleAnywhere, Category = "Stealth")
	FStealthDetectionEvent LastDetectionEvent;

	// TODO: currently registers the owning actor's default location/bounds as the sensed point.
	// If sight detection ends up not respecting crouch correctly once tested in-engine, revisit
	// tying this to a socket (chest/head) that actually moves with CrouchedHalfHeight.
	UPROPERTY()
	TObjectPtr<class UAIPerceptionStimuliSourceComponent> StimuliSource;
};
