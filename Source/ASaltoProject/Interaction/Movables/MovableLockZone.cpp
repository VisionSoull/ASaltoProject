// Fill out your copyright notice in the Description page of Project Settings.

#include "MovableLockZone.h"
#include "MovableBase.h"
#include "Movable.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"

AMovableLockZone::AMovableLockZone()
{
	// Ticks only while a movable is inside
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	LockArea = CreateDefaultSubobject<UBoxComponent>(TEXT("LockArea"));
	SetRootComponent(LockArea);
	LockArea->SetBoxExtent(FVector(100.f, 100.f, 50.f));
	LockArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LockArea->SetCollisionResponseToAllChannels(ECR_Ignore);
	LockArea->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap); // "Interactable": the movables' object type
	LockArea->SetGenerateOverlapEvents(true);

	LockTarget = CreateDefaultSubobject<USceneComponent>(TEXT("LockTarget"));
	LockTarget->SetupAttachment(LockArea);
}

void AMovableLockZone::BeginPlay()
{
	Super::BeginPlay();
	LockArea->OnComponentBeginOverlap.AddDynamic(this, &AMovableLockZone::OnAreaBeginOverlap);
	LockArea->OnComponentEndOverlap.AddDynamic(this, &AMovableLockZone::OnAreaEndOverlap);

	// Overlap events only fire on entering: pick up movables that start inside the area
	TArray<AActor*> OverlappingActors;
	LockArea->GetOverlappingActors(OverlappingActors, AMovableBase::StaticClass());
	for (AActor* Actor : OverlappingActors)
	{
		Candidates.AddUnique(Cast<AMovableBase>(Actor));
	}
	SetActorTickEnabled(Candidates.Num() > 0);
}

void AMovableLockZone::OnAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AMovableBase* Movable = Cast<AMovableBase>(OtherActor);
	if (!Movable || LockedHere.Contains(Movable))
	{
		return;
	}

	Candidates.AddUnique(Movable);
	SetActorTickEnabled(true);
}

void AMovableLockZone::OnAreaEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	AMovableBase* Movable = Cast<AMovableBase>(OtherActor);
	if (!Movable)
	{
		return;
	}

	Candidates.Remove(Movable);
	LockedHere.Remove(Movable);
	if (Candidates.Num() == 0)
	{
		SetActorTickEnabled(false);
	}
}

void AMovableLockZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const FVector Target = LockTarget->GetComponentLocation();

	for (int32 Index = Candidates.Num() - 1; Index >= 0; --Index)
	{
		AMovableBase* Movable = Candidates[Index];
		if (!IsValid(Movable))
		{
			Candidates.RemoveAt(Index);
			continue;
		}

		// Not the movable this zone wants, or already locked by something else (puzzle, cutscene): leave it alone
		if ((RequiredMovable && Movable != RequiredMovable) || IMovable::Execute_IsLocked(Movable))
		{
			continue;
		}

		if (FVector::Dist2D(Movable->GetActorLocation(), Target) <= LockOffset)
		{
			Movable->Lock();
			Candidates.RemoveAt(Index);
			LockedHere.Add(Movable);
			OnMovableLocked.Broadcast(Movable);
		}
	}

	if (bDebugDraw)
	{
		DrawDebugCircle(GetWorld(), Target, LockOffset, 32, FColor::Yellow, false, -1.f, 0, 1.5f,
			FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
	}

	if (Candidates.Num() == 0)
	{
		SetActorTickEnabled(false);
	}
}