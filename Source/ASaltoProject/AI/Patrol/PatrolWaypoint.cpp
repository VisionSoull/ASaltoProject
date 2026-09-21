#include "PatrolWaypoint.h"
#include "Components/BillboardComponent.h"
#include "Components/SphereComponent.h"

APatrolWaypoint::APatrolWaypoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	RootComponent = SphereComp;
	SphereComp->InitSphereRadius(50.0f);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BillboardComp = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillboardComponent"));
	BillboardComp->SetupAttachment(RootComponent);
}

void APatrolWaypoint::BeginPlay()
{
	Super::BeginPlay();
}
