#include "World/RatSpawnPoints.h"

#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"

ARatSpawnPoint::ARatSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
#if WITH_EDITORONLY_DATA
	Preview = CreateEditorOnlyDefaultSubobject<UCapsuleComponent>(TEXT("Preview"));
	if (Preview)
	{
		Preview->SetupAttachment(RootComponent);
		Preview->InitCapsuleSize(30.f, 34.f);
		Preview->SetRelativeLocation(FVector(0.f, 0.f, 34.f));
		Preview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Preview->ShapeColor = FColor(80, 160, 255);
		Preview->bHiddenInGame = true;
	}
	Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (Arrow)
	{
		Arrow->SetupAttachment(RootComponent);
		Arrow->SetRelativeLocation(FVector(0.f, 0.f, 34.f));
		Arrow->ArrowColor = FColor(80, 160, 255);
	}
#endif
}

ACheeseSpawnPoint::ACheeseSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
#if WITH_EDITORONLY_DATA
	Preview = CreateEditorOnlyDefaultSubobject<UCapsuleComponent>(TEXT("Preview"));
	if (Preview)
	{
		Preview->SetupAttachment(RootComponent);
		Preview->InitCapsuleSize(26.f, 26.f);
		Preview->SetRelativeLocation(FVector(0.f, 0.f, 26.f));
		Preview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Preview->ShapeColor = FColor(255, 210, 40);
		Preview->bHiddenInGame = true;
	}
#endif
}
