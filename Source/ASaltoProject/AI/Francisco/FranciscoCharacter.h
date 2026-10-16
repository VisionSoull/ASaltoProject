#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FranciscoTypes.h"
#include "ProximityComponent.h"
#include "EmotionalStateComponent.h"
#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInput/Public/InputAction.h"
#include "FranciscoCharacter.generated.h"

class AFranciscoAIController;

UCLASS()
class ASALTOPROJECT_API AFranciscoCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AFranciscoCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components", meta = (AllowPrivateAccess = "true"))
	UProximityComponent* ProximityComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components", meta = (AllowPrivateAccess = "true"))
	UEmotionalStateComponent* EmotionalState;
	
	UPROPERTY()
	APawn* PlayerReference;

	UPROPERTY()
	AFranciscoAIController* FranciscoAI;

public:

	UFUNCTION(BlueprintCallable, Category="AI")
	APawn* GetPlayerReference() const { return PlayerReference; }

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetPlayerReference(APawn* NewPlayerReference);

	UFUNCTION(BlueprintCallable, Category = "AI")
	UProximityComponent* GetProximityComponent() const { return ProximityComponent; }

	UFUNCTION(BlueprintCallable, Category = "AI")
	UEmotionalStateComponent* GetEmotionalStateComponent() const { return EmotionalState; }

	UFUNCTION(BlueprintCallable, Category = "AI")
	float GetDistanceToPlayer() const;

	UFUNCTION(BlueprintCallable, Category = "AI")
	float GetStressLevel() const;

	UFUNCTION(BlueprintCallable, Category = "AI")
	EEmotionalState GetEmotionalState() const;

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void TestComponentCommunication();

	UFUNCTION()
	void OnHoldHandAction(const FInputActionInstance& Instance);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* HoldHandInputAction;

protected:
	void FindPlayerReference();
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


};
