#pragma once

#include "CoreMinimal.h"
#include "Game/RatuchoTypes.h"
#include "GameFramework/Actor.h"
#include "RatProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

/** Dados visuais replicados (os clientes montam a aparência a partir deles). */
USTRUCT()
struct FRatProjectileVisual
{
	GENERATED_BODY()

	UPROPERTY() ERatWeaponType Weapon = ERatWeaponType::Slingshot;
	UPROPERTY() float GravityScale = 0.f;
	UPROPERTY() FVector_NetQuantize10 Scale = FVector(0.1f);
	UPROPERTY() FLinearColor Color = FLinearColor::White;
};

/**
 * Projétil replicado. Colisão e dano são decididos SOMENTE no servidor;
 * nos clientes é só apresentação (movimento + efeito de impacto).
 */
UCLASS()
class RATUCHO_API ARatProjectile : public AActor
{
	GENERATED_BODY()

public:
	ARatProjectile();

	/** Servidor, antes de FinishSpawning. */
	void InitProjectile(const FRatWeaponStats& Stats, ERatWeaponType Weapon, const FVector& Direction, AController* Shooter);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Projectile")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Projectile")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Projectile")
	TObjectPtr<UProjectileMovementComponent> Movement;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_Visual();

	UFUNCTION()
	void HandleStop(const FHitResult& Hit);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastImpact(FVector_NetQuantize Location, bool bHitRat);

private:
	void ApplyVisual();

	UPROPERTY(ReplicatedUsing = OnRep_Visual)
	FRatProjectileVisual Visual;

	float Damage = 0.f;
	bool bImpacted = false;

	/** Quem disparou (para crédito da eliminação mesmo que o atirador morra antes do impacto). */
	TWeakObjectPtr<AController> ShooterController;
};

/** Efeito cosmético local de impacto (esfera que expande e some). Não replicado. */
UCLASS()
class RATUCHO_API ARatImpactFX : public AActor
{
	GENERATED_BODY()

public:
	ARatImpactFX();
	void Init(const FLinearColor& Color, float Size);
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh;

	float Age = 0.f;
	float Lifetime = 0.25f;
	float MaxSize = 0.6f;
};
