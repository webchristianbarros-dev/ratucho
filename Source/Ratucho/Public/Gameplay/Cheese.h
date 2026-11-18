#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Cheese.generated.h"

class ARatCharacter;
class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UProjectileMovementComponent;
struct FHitResult;

/**
 * O único queijo da partida. Toda transição (livre → carregado → solto/lançado)
 * acontece SOMENTE no servidor; clientes recebem Carrier/posição replicados.
 *
 * Regras:
 * - Nunca há duas posses: TryPickup só aceita se Carrier == nullptr.
 * - Morte/desconexão do portador: cai no local, com correção para não entrar na geometria.
 * - Tecla G: lançado com pequena trajetória e fica onde aterrissar.
 * - Nunca é teletransportado para um spawn depois de solto (exceto se cair fora do mapa).
 */
UCLASS()
class RATUCHO_API ACheese : public AActor
{
	GENERATED_BODY()

public:
	ACheese();

	UFUNCTION(BlueprintPure, Category = "Ratucho|Cheese")
	ARatCharacter* GetCarrier() const { return Carrier; }

	UFUNCTION(BlueprintPure, Category = "Ratucho|Cheese")
	bool IsFree() const { return Carrier == nullptr; }

	/** Servidor: tenta entregar o queijo a Rat. Retorna true se a posse mudou. */
	bool TryPickup(ARatCharacter* Rat, bool bRequireRange = true);

	/** Servidor: o portador lança o queijo (tecla G). */
	void Throw(ARatCharacter* From);

	/** Servidor: o portador morreu/saiu; o queijo cai em DropLocation. */
	void DropAt(const FVector& DropLocation, bool bFromLeave);

	/** Servidor: posiciona o queijo livre apoiado no chão (spawn inicial). */
	void PlaceOnGround(const FVector& Location);

	/** Distância máxima para o jogador pegar com E (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Cheese")
	float PickupRange = 190.f;

	/** Distância em que bots pegam automaticamente ao alcançar o queijo (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Cheese")
	float BotAutoPickupRange = 110.f;

	/** Velocidade horizontal/vertical do lançamento com G. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Cheese")
	float ThrowForwardSpeed = 750.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Cheese")
	float ThrowUpSpeed = 420.f;

	/** Quem soltou não pode pegar de novo por este tempo (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Cheese")
	float RepickCooldown = 0.8f;

	/** Abaixo desta altura o queijo é considerado fora do mapa (fallback de segurança). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Cheese")
	float OutOfWorldZ = -2000.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Cheese")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Cheese")
	TObjectPtr<USceneComponent> VisualRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Cheese")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Cheese")
	TObjectPtr<UPointLightComponent> Glow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Cheese")
	TObjectPtr<UProjectileMovementComponent> Movement;

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_Carrier(ARatCharacter* OldCarrier);

	UFUNCTION()
	void HandleProjectileStop(const FHitResult& ImpactResult);

private:
	void ApplyCarriedState(bool bCarried);
	FVector FindSafeGroundLocation(const FVector& Desired) const;
	void ServerTickAutoPickup();

	UPROPERTY(ReplicatedUsing = OnRep_Carrier)
	TObjectPtr<ARatCharacter> Carrier = nullptr;

	TWeakObjectPtr<ARatCharacter> LastDropper;
	double LastDropTime = -100.0;
	FVector LastSafeLocation = FVector::ZeroVector;
	float BobTime = 0.f;
};
