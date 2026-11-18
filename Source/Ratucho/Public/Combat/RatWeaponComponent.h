#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Game/RatuchoTypes.h"
#include "RatWeaponComponent.generated.h"

class ARatCharacter;
class ARatProjectile;

/**
 * Arma do rato. O cliente só pede o disparo (ServerFire com a direção da mira);
 * o servidor valida vida, fase da rodada, cadência, munição, recarga e
 * plausibilidade da direção, e então cria os projéteis replicados.
 * Bots chamam FireFromServer diretamente — mesmas regras.
 */
UCLASS(ClassGroup = (Ratucho), meta = (BlueprintSpawnableComponent))
class RATUCHO_API URatWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URatWeaponComponent();

	/** Parâmetros de balanceamento das duas armas (editáveis no Blueprint do personagem). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ratucho|Weapon")
	FRatWeaponStats SlingshotStats;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ratucho|Weapon")
	FRatWeaponStats CapBlasterStats;

	/** Desvio máximo aceito entre a direção pedida e a rotação de controle (graus). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ratucho|Weapon")
	float MaxAimDeviationDegrees = 30.f;

	UFUNCTION(BlueprintPure, Category = "Ratucho|Weapon")
	const FRatWeaponStats& GetStats() const;

	UFUNCTION(BlueprintPure, Category = "Ratucho|Weapon")
	ERatWeaponType GetWeaponType() const { return WeaponType; }

	UFUNCTION(BlueprintPure, Category = "Ratucho|Weapon")
	int32 GetAmmo() const { return Ammo; }

	UFUNCTION(BlueprintPure, Category = "Ratucho|Weapon")
	bool IsReloading() const { return bReloading; }

	/** 0..1 do progresso da recarga (para o HUD). */
	float GetReloadProgress() const;

	/** Servidor: troca a arma e enche o pente. */
	void ServerSetWeapon(ERatWeaponType NewType);

	/** Local (jogador): gatilho pressionado/solto. */
	void SetTriggerHeld(bool bHeld);

	/** Local (jogador): pede recarga. */
	void RequestReload();

	/** Servidor: disparo de bot (ou chamado após validação de ServerFire). Retorna true se disparou. */
	bool FireFromServer(const FVector& AimDirection);

	/** Ponto de saída dos projéteis (calculado igual em cliente e servidor). */
	FVector GetMuzzleLocation() const;

	/** Servidor: cancela rajadas e recarga (morte/fim de rodada). */
	void ServerResetState();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Classe do projétil. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ratucho|Weapon")
	TSubclassOf<ARatProjectile> ProjectileClass;

protected:
	UFUNCTION(Server, Reliable)
	void ServerFire(FVector_NetQuantizeNormal AimDirection);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	UFUNCTION()
	void OnRep_WeaponType();

private:
	ARatCharacter* GetRat() const;
	bool CanFireNow(double Now) const;
	void SpawnProjectile(const FVector& AimDirection);
	void FireBurstShot();
	void StartReload();
	void FinishReload();
	FVector ComputeLocalAimDirection() const;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponType)
	ERatWeaponType WeaponType = ERatWeaponType::Slingshot;

	UPROPERTY(Replicated)
	int32 Ammo = 0;

	UPROPERTY(Replicated)
	bool bReloading = false;

	UPROPERTY(Replicated)
	float ReloadStartServerTime = 0.f;

	bool bTriggerHeld = false;
	double LastLocalFireTime = -100.0;
	double LastServerFireTime = -100.0;
	int32 PendingBurstShots = 0;
	FVector PendingBurstDirection = FVector::ForwardVector;
	FTimerHandle BurstTimer;
	FTimerHandle ReloadTimer;
};
