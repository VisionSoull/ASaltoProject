// Fill out your copyright notice in the Description page of Project Settings.

#include "ProximityComponent.h"
#include "FranciscoCharacter.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

UProximityComponent::UProximityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Default distance thresholds
	ComfortDistance = 100.0f;
	NormalDistance = 300.0f;
	WarningDistance = 500.0f;
	DangerDistance = 800.0f;

	ReconnectDistance = 150.0f;
	MaxHandHoldDistance = 200.0f;

	ComfortSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ComfortSphere"));
	NormalSphere = CreateDefaultSubobject<USphereComponent>(TEXT("NormalSphere"));
	WarningSphere = CreateDefaultSubobject<USphereComponent>(TEXT("WarningSphere"));
	DangerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DangerSphere"));

	for (USphereComponent* Sphere : { ComfortSphere, NormalSphere, WarningSphere, DangerSphere })
	{
		Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Sphere->SetCollisionObjectType(ECC_WorldDynamic);
		Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Sphere->SetGenerateOverlapEvents(true);
	}

	bPlayerInComfort = false;
	bPlayerInNormal = false;
	bPlayerInWarning = false;
	bPlayerInDanger = false;

	bInHandHoldRange = false;
	bInitialSyncDone = false;
	bUpdatingSpheres = false;

	UpdateInterval = 0.2f;
	bDebugDrawDistance = false;

	CurrentDistanceToPlayer = 0.0f;
	CurrentZone = EProximityZone::Normal;
	LastDistance = 0.0f;
	LastZone = EProximityZone::Normal;
	UpdateTimer = 0.0f;

	OwnerCharacter = nullptr;
}

void UProximityComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AFranciscoCharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("ProximityComponent: Owner is not AFranciscoCharacter!"));
		return;
	}

	USphereComponent* Spheres[4] = { ComfortSphere, NormalSphere, WarningSphere, DangerSphere };
	const float Radii[4] = { ComfortDistance, NormalDistance, WarningDistance, DangerDistance };

	USceneComponent* AttachTarget = OwnerCharacter->GetRootComponent();

	for (int32 i = 0; i < 4; i++)
	{
		USphereComponent* Sphere = Spheres[i];
		if (!Sphere) continue;

		if (!Sphere->IsRegistered())
		{
			Sphere->RegisterComponent();
		}

		if (AttachTarget)
		{
			Sphere->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform);
		}

		Sphere->SetSphereRadius(Radii[i]);
	}

	ComfortSphere->OnComponentBeginOverlap.AddDynamic(this, &UProximityComponent::OnComfortSphereBeginOverlap);
	ComfortSphere->OnComponentEndOverlap.AddDynamic(this, &UProximityComponent::OnComfortSphereEndOverlap);
	NormalSphere->OnComponentBeginOverlap.AddDynamic(this, &UProximityComponent::OnNormalSphereBeginOverlap);
	NormalSphere->OnComponentEndOverlap.AddDynamic(this, &UProximityComponent::OnNormalSphereEndOverlap);
	WarningSphere->OnComponentBeginOverlap.AddDynamic(this, &UProximityComponent::OnWarningSphereBeginOverlap);
	WarningSphere->OnComponentEndOverlap.AddDynamic(this, &UProximityComponent::OnWarningSphereEndOverlap);
	DangerSphere->OnComponentBeginOverlap.AddDynamic(this, &UProximityComponent::OnDangerSphereBeginOverlap);
	DangerSphere->OnComponentEndOverlap.AddDynamic(this, &UProximityComponent::OnDangerSphereEndOverlap);

	UE_LOG(LogTemp, Log, TEXT("ProximityComponent initialized (sphere-based)"));
}

void UProximityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bInitialSyncDone && OwnerCharacter)
	{
		if (APawn* Player = OwnerCharacter->GetPlayerReference())
		{
			const float D = FVector::Dist(OwnerCharacter->GetActorLocation(), Player->GetActorLocation());
			bPlayerInComfort = D <= ComfortDistance;
			bPlayerInNormal = D <= NormalDistance;
			bPlayerInWarning = D <= WarningDistance;
			bPlayerInDanger = D <= DangerDistance;

			bInitialSyncDone = true;
			UpdateZoneFromOverlaps();
		}
	}

	if (!bDebugDrawDistance) return;

	UpdateTimer += DeltaTime;
	if (UpdateTimer >= UpdateInterval)
	{
		DrawDebugInfo();
		UpdateTimer = 0.0f;
	}
}

bool UProximityComponent::ShouldPanic() const
{
	return CurrentZone == EProximityZone::Panic;
}

void UProximityComponent::OnStateChanged(EFranciscoState OldState, EFranciscoState NewState)
{
	UE_LOG(LogTemp, Log, TEXT("ProximityComponent: Aware of state change %s -> %s"),
		*UEnum::GetValueAsString(OldState),
		*UEnum::GetValueAsString(NewState));
}

// -------------------------------------------------------------------------
// Overlap handling
// -------------------------------------------------------------------------

bool UProximityComponent::IsRelevantOverlap(AActor* OtherActor, UPrimitiveComponent* OtherComp) const
{
	if (bUpdatingSpheres || !OwnerCharacter) return false;

	APawn* Player = OwnerCharacter->GetPlayerReference();
	if (!Player || OtherActor != Player) return false;

	const ACharacter* PlayerChar = Cast<ACharacter>(Player);
	return PlayerChar && OtherComp == PlayerChar->GetCapsuleComponent();
}

// Comfort
void UProximityComponent::OnComfortSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsRelevantOverlap(OtherActor, OtherComp)) return;
	bPlayerInComfort = true;
	UpdateZoneFromOverlaps();
}

void UProximityComponent::OnComfortSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!IsRelevantOverlap(OtherActor, OtherComp)) return;
	bPlayerInComfort = false;
	UpdateZoneFromOverlaps();
}

// Normal
void UProximityComponent::OnNormalSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsRelevantOverlap(OtherActor, OtherComp)) return;
	bPlayerInNormal = true;
	UpdateZoneFromOverlaps();
}

void UProximityComponent::OnNormalSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!IsRelevantOverlap(OtherActor, OtherComp)) return;
	bPlayerInNormal = false;
	UpdateZoneFromOverlaps();
}

// Warning
void UProximityComponent::OnWarningSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsRelevantOverlap(OtherActor, OtherComp)) return;
	bPlayerInWarning = true;
	UpdateZoneFromOverlaps();
}

void UProximityComponent::OnWarningSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!IsRelevantOverlap(OtherActor, OtherComp)) return;
	bPlayerInWarning = false;
	UpdateZoneFromOverlaps();
}

// Danger
void UProximityComponent::OnDangerSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsRelevantOverlap(OtherActor, OtherComp)) return;
	bPlayerInDanger = true;
	UpdateZoneFromOverlaps();
}

void UProximityComponent::OnDangerSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!IsRelevantOverlap(OtherActor, OtherComp)) return;
	bPlayerInDanger = false;
	UpdateZoneFromOverlaps();
}

// -------------------------------------------------------------------------
// Zone logic
// -------------------------------------------------------------------------

void UProximityComponent::UpdateZoneFromOverlaps()
{
	LastZone = CurrentZone;

	if (bPlayerInComfort)      CurrentZone = EProximityZone::Comfort;
	else if (bPlayerInNormal)  CurrentZone = EProximityZone::Normal;
	else if (bPlayerInWarning) CurrentZone = EProximityZone::Warning;
	else if (bPlayerInDanger)  CurrentZone = EProximityZone::Danger;
	else                       CurrentZone = EProximityZone::Panic;

	if (bPlayerInComfort)
	{
		bInHandHoldRange = true;
	}
	else if (!bPlayerInNormal)
	{
		bInHandHoldRange = false;
	}

	if (OwnerCharacter)
	{
		if (APawn* Player = OwnerCharacter->GetPlayerReference())
		{
			LastDistance = CurrentDistanceToPlayer;
			CurrentDistanceToPlayer = FVector::Dist(OwnerCharacter->GetActorLocation(), Player->GetActorLocation());

			if (FMath::Abs(CurrentDistanceToPlayer - LastDistance) > 10.0f)
			{
				OnProximityChanged.Broadcast(LastDistance, CurrentDistanceToPlayer);
			}
		}
	}

	if (CurrentZone != LastZone)
	{
		OnZoneChanged.Broadcast(CurrentZone);
		UE_LOG(LogTemp, Log, TEXT("ProximityComponent: Zone changed from %s to %s"),
			*UEnum::GetValueAsString(LastZone), *UEnum::GetValueAsString(CurrentZone));
	}

	UpdateActiveSpheres();
}

void UProximityComponent::UpdateActiveSpheres()
{
	int32 Z = 0;
	switch (CurrentZone)
	{
	case EProximityZone::Comfort: Z = 0; break;
	case EProximityZone::Normal:  Z = 1; break;
	case EProximityZone::Warning: Z = 2; break;
	case EProximityZone::Danger:  Z = 3; break;
	case EProximityZone::Panic:   Z = 4; break;
	}

	USphereComponent* Spheres[4] = { ComfortSphere, NormalSphere, WarningSphere, DangerSphere };
	bool* Flags[4] = { &bPlayerInComfort, &bPlayerInNormal, &bPlayerInWarning, &bPlayerInDanger };

	bUpdatingSpheres = true;
	for (int32 i = 0; i < 4; i++)
	{
		if (!Spheres[i]) continue;

		const bool bActive = (i >= Z - 1) && (i <= Z + 1);
		Spheres[i]->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		*Flags[i] = (i >= Z);
	}
	bUpdatingSpheres = false;
}

// -------------------------------------------------------------------------
// Debug
// -------------------------------------------------------------------------

void UProximityComponent::DrawDebugInfo() const
{
	if (!OwnerCharacter) return;

	const FVector OwnerLocation = OwnerCharacter->GetActorLocation();

	struct FSphereDebugEntry
	{
		USphereComponent* Sphere;
		FColor ActiveColor;
	};

	const FSphereDebugEntry Entries[4] = {
		{ ComfortSphere, FColor::Blue },
		{ NormalSphere,  FColor::Green },
		{ WarningSphere, FColor::Yellow },
		{ DangerSphere,  FColor::Orange },
	};

	for (const FSphereDebugEntry& Entry : Entries)
	{
		if (!Entry.Sphere) continue;

		const bool bActive = Entry.Sphere->GetCollisionEnabled() != ECollisionEnabled::NoCollision;
		const FColor DrawColor = bActive ? Entry.ActiveColor : FColor(120, 120, 120);

		DrawDebugSphere(GetWorld(), OwnerLocation, Entry.Sphere->GetScaledSphereRadius(), 24, DrawColor, false, UpdateInterval);
	}

	if (APawn* Player = OwnerCharacter->GetPlayerReference())
	{
		FColor LineColor = FColor::Green;
		switch (CurrentZone)
		{
		case EProximityZone::Comfort:  LineColor = FColor::Blue; break;
		case EProximityZone::Normal:   LineColor = FColor::Green; break;
		case EProximityZone::Warning:  LineColor = FColor::Yellow; break;
		case EProximityZone::Danger:   LineColor = FColor::Orange; break;
		case EProximityZone::Panic:    LineColor = FColor::Red; break;
		}

		DrawDebugLine(GetWorld(), OwnerLocation, Player->GetActorLocation(), LineColor, false, UpdateInterval);

		const FString Text = FString::Printf(TEXT("Distance: %.1f\nZone: %s\nHandRange: %s"),
			CurrentDistanceToPlayer,
			*UEnum::GetValueAsString(CurrentZone),
			bInHandHoldRange ? TEXT("Yes") : TEXT("No"));
		DrawDebugString(GetWorld(), OwnerLocation + FVector(0, 0, 120), Text, nullptr, LineColor, UpdateInterval);
	}
}