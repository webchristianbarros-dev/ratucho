#include "Combat/RatProjectile.h"

#include "Characters/RatCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Player/RatPlayerController.h"
#include "UI/RatSfx.h"
#include "UObject/ConstructorHelpers.h"

ARatProjectile::ARatProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(60.f);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(9.f);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Collision->SetCanEverAffectNavigation(false);
	RootComponent = Collision;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cyl.Succeeded())
	{
		Mesh->SetStaticMesh(Cyl.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SolidMat(TEXT("/Game/Ratucho/Materials/Master/M_RatSolid.M_RatSolid"));
	if (SolidMat.Succeeded())
	{
		Mesh->SetMaterial(0, SolidMat.Object);
	}

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->SetUpdatedComponent(Collision);
	Movement->bRotationFollowsVelocity = true;
	Movement->bShouldBounce = false;
	Movement->InitialSpeed = 3600.f;
	Movement->MaxSpeed = 8000.f;
	Movement->ProjectileGravityScale = 0.35f;

	InitialLifeSpan = 2.f;
}

void ARatProjectile::InitProjectile(const FRatWeaponStats& Stats, ERatWeaponType Weapon, const FVector& Direction, AController* Shooter)
{
	ShooterController = Shooter;
	Damage = Stats.Damage;
	Collision->SetSphereRadius(Stats.ProjectileRadius);
	Movement->InitialSpeed = Stats.ProjectileSpeed;
	Movement->MaxSpeed = Stats.ProjectileSpeed * 1.5f;
	Movement->ProjectileGravityScale = Stats.ProjectileGravityScale;
	Movement->Velocity = Direction.GetSafeNormal() * Stats.ProjectileSpeed;
	InitialLifeSpan = Stats.MaxRange / FMath::Max(1.f, Stats.ProjectileSpeed);

	Visual.Weapon = Weapon;
	Visual.GravityScale = Stats.ProjectileGravityScale;
	Visual.Scale = Stats.ProjectileScale;
	Visual.Color = Stats.ProjectileColor;
}

void ARatProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ARatProjectile, Visual, COND_InitialOnly);
}

void ARatProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* Inst = GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(Inst, true);
	}
	if (HasAuthority())
	{
		Movement->OnProjectileStop.AddDynamic(this, &ARatProjectile::HandleStop);
	}
	else
	{
		// No cliente, o projétil só "anda"; quem decide o impacto é o servidor.
		Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
	ApplyVisual();

	const FVector Loc = GetActorLocation();
	RatSfx::PlayCue(this, Visual.Weapon == ERatWeaponType::CapBlaster ? ERatSfx::FireCaps : ERatSfx::FireSlingshot, &Loc);
}

void ARatProjectile::OnRep_Visual()
{
	ApplyVisual();
}

void ARatProjectile::ApplyVisual()
{
	Movement->ProjectileGravityScale = Visual.GravityScale;
	Mesh->SetRelativeScale3D(Visual.Scale);
	// Rolha: cilindro deitado no sentido do voo. Tampinha: disco girando.
	Mesh->SetRelativeRotation(Visual.Weapon == ERatWeaponType::CapBlaster ? FRotator(0.f, 0.f, 0.f) : FRotator(90.f, 0.f, 0.f));
	if (UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Visual.Color);
	}
}

void ARatProjectile::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	if (Movement)
	{
		Movement->Velocity = NewVelocity;
	}
}

void ARatProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Visual.Weapon == ERatWeaponType::CapBlaster && Mesh)
	{
		Mesh->AddLocalRotation(FRotator(0.f, 1440.f * DeltaSeconds, 0.f));
	}
}

void ARatProjectile::HandleStop(const FHitResult& Hit)
{
	if (bImpacted)
	{
		return;
	}
	bImpacted = true;

	bool bHitRat = false;
	if (ARatCharacter* Rat = Cast<ARatCharacter>(Hit.GetActor()))
	{
		AController* InstController = ShooterController.Get();
		FPointDamageEvent Event(Damage, Hit, GetVelocity().GetSafeNormal(), nullptr);
		const float Applied = Rat->TakeDamage(Damage, Event, InstController, this);
		bHitRat = Applied > 0.f;
		if (bHitRat)
		{
			if (ARatPlayerController* PC = Cast<ARatPlayerController>(InstController))
			{
				PC->ClientHitConfirmed(Rat->IsDead());
			}
		}
	}

	MulticastImpact(Hit.ImpactPoint, bHitRat);
	SetActorHiddenInGame(true);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetLifeSpan(0.1f);
}

void ARatProjectile::MulticastImpact_Implementation(FVector_NetQuantize Location, bool bHitRat)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	SetActorHiddenInGame(true);
	const FVector Loc = Location;
	RatSfx::PlayCue(this, bHitRat ? ERatSfx::HitRat : ERatSfx::HitWorld, &Loc);
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ARatImpactFX* FX = GetWorld()->SpawnActor<ARatImpactFX>(Loc, FRotator::ZeroRotator, P))
	{
		FX->Init(bHitRat ? FLinearColor(1.f, 0.35f, 0.2f) : FLinearColor(0.95f, 0.9f, 0.75f), bHitRat ? 0.9f : 0.45f);
	}
}

// ============================================================================

ARatImpactFX::ARatImpactFX()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	RootComponent = Mesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		Mesh->SetStaticMesh(Sphere.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SolidMat(TEXT("/Game/Ratucho/Materials/Master/M_RatSolid.M_RatSolid"));
	if (SolidMat.Succeeded())
	{
		Mesh->SetMaterial(0, SolidMat.Object);
	}
	InitialLifeSpan = 0.4f;
}

void ARatImpactFX::Init(const FLinearColor& Color, float Size)
{
	MaxSize = Size;
	if (UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("Emissive"), 2.f);
	}
	SetActorScale3D(FVector(0.05f));
}

void ARatImpactFX::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	const float A = FMath::Clamp(Age / Lifetime, 0.f, 1.f);
	const float S = FMath::Lerp(0.05f, MaxSize, FMath::Sqrt(A)) * (1.f - A * 0.6f);
	SetActorScale3D(FVector(FMath::Max(0.01f, S)));
	if (A >= 1.f)
	{
		Destroy();
	}
}
