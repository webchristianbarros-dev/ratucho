#include "Gameplay/Cheese.h"

#include "Characters/RatCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/RatuchoGameMode.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "RatuchoLog.h"
#include "UObject/ConstructorHelpers.h"

ACheese::ACheese()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(30.f);
	bAlwaysRelevant = true; // o HUD de todos precisa saber onde está

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(26.f);
	Collision->SetCollisionObjectType(ECC_PhysicsBody);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCanEverAffectNavigation(false);
	RootComponent = Collision;

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(Collision);

	// Fatia de queijo provisória: cunha (cilindro de 3 lados não existe nas formas
	// básicas, então usamos um cubo achatado inclinado + brilho).
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(VisualRoot);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.5f, 0.38f, 0.32f));
	Mesh->SetRelativeRotation(FRotator(0.f, 25.f, 0.f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SolidMat(TEXT("/Game/Ratucho/Materials/Master/M_RatSolid.M_RatSolid"));
	if (SolidMat.Succeeded())
	{
		Mesh->SetMaterial(0, SolidMat.Object);
	}

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(VisualRoot);
	Glow->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	Glow->SetIntensity(1800.f);
	Glow->SetAttenuationRadius(260.f);
	Glow->SetLightColor(FLinearColor(1.f, 0.8f, 0.25f));
	Glow->SetCastShadows(false);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->SetUpdatedComponent(Collision);
	Movement->bAutoActivate = false;
	Movement->bShouldBounce = true;
	Movement->Bounciness = 0.25f;
	Movement->Friction = 0.6f;
	Movement->BounceVelocityStopSimulatingThreshold = 60.f;
	Movement->ProjectileGravityScale = 1.f;
	Movement->InitialSpeed = 0.f;
	Movement->MaxSpeed = 3000.f;
}

void ACheese::BeginPlay()
{
	Super::BeginPlay();
	LastSafeLocation = GetActorLocation();
	if (HasAuthority())
	{
		Movement->OnProjectileStop.AddDynamic(this, &ACheese::HandleProjectileStop);
	}
	if (UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.78f, 0.12f));
		MID->SetScalarParameterValue(TEXT("Emissive"), 0.35f);
		MID->SetScalarParameterValue(TEXT("Roughness"), 0.45f);
	}
}

void ACheese::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACheese, Carrier);
}

bool ACheese::TryPickup(ARatCharacter* Rat, bool bRequireRange)
{
	if (!HasAuthority() || !Rat || !IsFree() || Rat->IsDead())
	{
		return false;
	}
	if (ARatuchoGameMode* GM = GetWorld()->GetAuthGameMode<ARatuchoGameMode>())
	{
		if (!GM->IsRoundInProgress())
		{
			return false;
		}
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (LastDropper.Get() == Rat && Now - LastDropTime < RepickCooldown)
	{
		return false;
	}
	if (bRequireRange && FVector::Dist(Rat->GetActorLocation(), GetActorLocation()) > PickupRange)
	{
		return false;
	}

	Movement->StopMovementImmediately();
	Movement->Deactivate();
	ARatCharacter* Old = Carrier;
	Carrier = Rat;
	AttachToComponent(Rat->GetCheeseSocket(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	ApplyCarriedState(true);
	OnRep_Carrier(Old);

	if (ARatuchoGameMode* GM = GetWorld()->GetAuthGameMode<ARatuchoGameMode>())
	{
		GM->NotifyCheeseEvent(ERatCheeseEvent::PickedUp, Rat->GetPlayerState());
	}
	return true;
}

void ACheese::Throw(ARatCharacter* From)
{
	if (!HasAuthority() || !From || Carrier != From)
	{
		return;
	}
	ARatCharacter* Old = Carrier;
	Carrier = nullptr;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	ApplyCarriedState(false);

	// Sai à frente do rato, num ponto livre (sem atravessar parede).
	const FVector Fwd = From->GetActorForwardVector();
	FVector Start = From->GetActorLocation() + FVector(0.f, 0.f, 30.f);
	FVector Desired = Start + Fwd * 50.f;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CheeseThrow), false, this);
	Params.AddIgnoredActor(From);
	if (GetWorld()->SweepSingleByChannel(Hit, Start, Desired, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(Collision->GetScaledSphereRadius()), Params))
	{
		Desired = Hit.Location;
	}
	SetActorLocation(Desired, false, nullptr, ETeleportType::TeleportPhysics);

	LastDropper = From;
	LastDropTime = GetWorld()->GetTimeSeconds();

	Movement->SetUpdatedComponent(Collision);
	Movement->Velocity = Fwd * ThrowForwardSpeed + FVector::UpVector * ThrowUpSpeed + From->GetVelocity() * 0.5f;
	Movement->Activate(true);
	OnRep_Carrier(Old);

	if (ARatuchoGameMode* GM = GetWorld()->GetAuthGameMode<ARatuchoGameMode>())
	{
		GM->NotifyCheeseEvent(ERatCheeseEvent::Thrown, From->GetPlayerState());
	}
}

void ACheese::DropAt(const FVector& DropLocation, bool bFromLeave)
{
	if (!HasAuthority() || IsFree())
	{
		return;
	}
	ARatCharacter* Old = Carrier;
	APlayerState* OldPS = Old ? Old->GetPlayerState() : nullptr;
	Carrier = nullptr;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	ApplyCarriedState(false);
	LastDropper = Old;
	LastDropTime = GetWorld()->GetTimeSeconds();
	PlaceOnGround(DropLocation);
	OnRep_Carrier(Old);

	if (ARatuchoGameMode* GM = GetWorld()->GetAuthGameMode<ARatuchoGameMode>())
	{
		GM->NotifyCheeseEvent(bFromLeave ? ERatCheeseEvent::DroppedOnLeave : ERatCheeseEvent::DroppedOnDeath, OldPS);
	}
}

void ACheese::PlaceOnGround(const FVector& Location)
{
	if (!HasAuthority())
	{
		return;
	}
	Movement->StopMovementImmediately();
	Movement->Deactivate();
	const FVector Safe = FindSafeGroundLocation(Location);
	SetActorLocation(Safe, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(FRotator::ZeroRotator);
	LastSafeLocation = Safe;
}

FVector ACheese::FindSafeGroundLocation(const FVector& Desired) const
{
	UWorld* World = GetWorld();
	const float R = Collision->GetScaledSphereRadius();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CheeseGround), false, this);
	if (Carrier)
	{
		Params.AddIgnoredActor(Carrier);
	}

	// Desce até o chão a partir de um pouco acima do ponto desejado.
	FVector Result = Desired;
	FHitResult Hit;
	const FVector Start = Desired + FVector(0.f, 0.f, 60.f);
	const FVector End = Desired - FVector(0.f, 0.f, 3000.f);
	if (World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(R), Params) && !Hit.bStartPenetrating)
	{
		Result = Hit.Location + FVector(0.f, 0.f, 2.f);
	}

	// Correção final de penetração (paredes, cantos).
	FVector Adjusted = Result;
	FRotator Rot = FRotator::ZeroRotator;
	if (World->FindTeleportSpot(this, Adjusted, Rot))
	{
		Result = Adjusted;
	}
	return Result;
}

void ACheese::HandleProjectileStop(const FHitResult& ImpactResult)
{
	// Aterrissou depois do lançamento: fica exatamente ali, apoiado no chão.
	PlaceOnGround(GetActorLocation());
}

void ACheese::ApplyCarriedState(bool bCarried)
{
	Collision->SetCollisionEnabled(bCarried ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
	VisualRoot->SetRelativeScale3D(bCarried ? FVector(0.75f) : FVector(1.f));
	Glow->SetIntensity(bCarried ? 900.f : 1800.f);
}

void ACheese::OnRep_Carrier(ARatCharacter* OldCarrier)
{
	ApplyCarriedState(Carrier != nullptr);
	if (OldCarrier)
	{
		OldCarrier->OnCheeseCarryChanged(false);
	}
	if (Carrier)
	{
		Carrier->OnCheeseCarryChanged(true);
	}
}

void ACheese::ServerTickAutoPickup()
{
	if (!IsFree() || Movement->IsActive())
	{
		return;
	}
	ARatCharacter* Best = nullptr;
	float BestDist = BotAutoPickupRange;
	for (ARatCharacter* Rat : TActorRange<ARatCharacter>(GetWorld()))
	{
		if (Rat && Rat->IsBotControlled() && !Rat->IsDead())
		{
			const float D = FVector::Dist(Rat->GetActorLocation(), GetActorLocation());
			if (D < BestDist)
			{
				BestDist = D;
				Best = Rat;
			}
		}
	}
	if (Best)
	{
		TryPickup(Best, false);
	}
}

void ACheese::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		ServerTickAutoPickup();

		// Segurança: se cair para fora do mundo, volta ao último ponto seguro.
		if (IsFree() && GetActorLocation().Z < OutOfWorldZ)
		{
			UE_LOG(LogRatucho, Warning, TEXT("Queijo saiu do mundo; restaurando último ponto seguro."));
			PlaceOnGround(LastSafeLocation);
		}
		else if (IsFree() && !Movement->IsActive())
		{
			LastSafeLocation = GetActorLocation();
		}
	}

	// Cosmético: gira e flutua quando livre.
	BobTime += DeltaSeconds;
	if (IsFree())
	{
		VisualRoot->SetRelativeLocation(FVector(0.f, 0.f, 8.f + FMath::Sin(BobTime * 3.f) * 6.f));
		VisualRoot->AddLocalRotation(FRotator(0.f, 90.f * DeltaSeconds, 0.f));
	}
	else
	{
		VisualRoot->SetRelativeLocation(FVector::ZeroVector);
	}
}
