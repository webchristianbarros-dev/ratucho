#include "World/ClimbVolume.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AClimbVolume::AClimbVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false; // geometria estática do mapa: existe igual em todas as máquinas

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetBoxExtent(FVector(30.f, 100.f, 150.f));
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision); // detecção é geométrica
	Box->SetGenerateOverlapEvents(false);
	Box->ShapeColor = FColor(60, 220, 90);
	RootComponent = Box;

#if WITH_EDITORONLY_DATA
	Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (Arrow)
	{
		Arrow->SetupAttachment(Box);
		Arrow->ArrowColor = FColor(60, 220, 90);
		Arrow->ArrowSize = 2.f;
	}
#endif

	Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
	Marker->SetupAttachment(Box);
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Marker->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		Marker->SetStaticMesh(PlaneMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ShapeMat.Succeeded())
	{
		Marker->SetMaterial(0, ShapeMat.Object);
	}
}

void AClimbVolume::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Faixa fina colada à parede (lado -X da caixa), do tamanho da face escalável.
	const FVector Ext = Box->GetUnscaledBoxExtent();
	Marker->SetVisibility(bShowMarker);
	Marker->SetRelativeLocation(FVector(-Ext.X + 1.f, 0.f, 0.f));
	Marker->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f)); // normal do plano (+Z) passa a apontar para +X
	Marker->SetRelativeScale3D(FVector(Ext.Z * 2.f / 100.f, Ext.Y * 2.f / 100.f * 0.35f, 1.f));

	if (UMaterialInstanceDynamic* MID = Marker->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.25f, 0.85f, 0.35f));
	}
}

FVector AClimbVolume::GetSurfaceNormal() const
{
	return GetActorForwardVector().GetSafeNormal2D();
}

bool AClimbVolume::ContainsCapsule(const FVector& Location, float Radius, float HalfHeight) const
{
	const FTransform& T = Box->GetComponentTransform();
	const FVector Local = T.InverseTransformPositionNoScale(Location);
	const FVector Ext = Box->GetScaledBoxExtent();
	return FMath::Abs(Local.X) <= Ext.X + Radius
		&& FMath::Abs(Local.Y) <= Ext.Y + Radius * 0.5f
		&& FMath::Abs(Local.Z) <= Ext.Z + HalfHeight;
}

float AClimbVolume::GetTopZ() const
{
	return Box->Bounds.Origin.Z + Box->Bounds.BoxExtent.Z;
}
