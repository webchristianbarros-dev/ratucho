#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RatMovementComponent.generated.h"

class AClimbVolume;

UENUM(BlueprintType)
enum class ERatCustomMovement : uint8
{
	None  = 0 UMETA(Hidden),
	Climb = 1 UMETA(DisplayName = "Climb"),
};

/**
 * Movimento do rato, construído sobre o Character Movement (predição e
 * correção de rede nativas da Unreal).
 *
 * - Corrida: flag enviada em cada saved move (FLAG_Custom_0), então o servidor
 *   simula a mesma velocidade que o cliente previu, sem correções.
 * - Escalada: modo MOVE_Custom/Climb, só dentro de AClimbVolume. Entrada e
 *   saída dependem apenas de posição + input, ambos presentes no saved move,
 *   portanto são deterministas em cliente e servidor.
 */
UCLASS()
class RATUCHO_API URatMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	friend class FSavedMove_Rat;

public:
	URatMovementComponent();

	// --- Corrida -----------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Ratucho|Movement")
	void SetSprinting(bool bNewSprinting) { bWantsToSprint = bNewSprinting; }

	UFUNCTION(BlueprintPure, Category = "Ratucho|Movement")
	bool IsSprinting() const;

	/** Velocidade máxima correndo (cm/s). Caminhada usa MaxWalkSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Movement|Sprint", meta = (ClampMin = "0"))
	float SprintSpeed = 760.f;

	/** Aceleração ao correr (cm/s²). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Movement|Sprint", meta = (ClampMin = "0"))
	float SprintAcceleration = 3200.f;

	/** Só corre se o input estiver alinhado com a direção do corpo (evita correr de costas). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Movement|Sprint", meta = (ClampMin = "-1", ClampMax = "1"))
	float SprintMinForwardDot = 0.3f;

	// --- Escalada ----------------------------------------------------------
	UFUNCTION(BlueprintPure, Category = "Ratucho|Movement")
	bool IsClimbing() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Movement|Climb", meta = (ClampMin = "0"))
	float ClimbSpeed = 280.f;

	/** Fração da velocidade usada para se mover lateralmente na parede. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Movement|Climb", meta = (ClampMin = "0", ClampMax = "1"))
	float ClimbLateralFactor = 0.7f;

	/** Quão "de frente" para a parede o input precisa estar para agarrar (cos do ângulo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Movement|Climb", meta = (ClampMin = "0", ClampMax = "1"))
	float ClimbEnterInputDot = 0.6f;

	/** Não agarra enquanto estiver subindo mais rápido que isso (ex.: logo após pular da parede). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Movement|Climb", meta = (ClampMin = "0"))
	float ClimbEnterMaxUpSpeed = 150.f;

	/** Impulso ao chegar no topo: para frente (sobre a plataforma) e para cima. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Movement|Climb")
	float MantleForwardSpeed = 320.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Movement|Climb")
	float MantleUpSpeed = 380.f;

	/** Pulo de parede: afastamento e altura. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Movement|Climb")
	float WallJumpOutSpeed = 480.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Movement|Climb")
	float WallJumpUpSpeed = 520.f;

	// --- UCharacterMovementComponent -----------------------------------------
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
	virtual bool CanAttemptJump() const override;
	virtual bool DoJump(bool bReplayingMoves, float DeltaTime) override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

protected:
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	virtual void PhysicsRotation(float DeltaTime) override;

private:
	void PhysClimb(float DeltaTime, int32 Iterations);
	AClimbVolume* FindClimbVolume() const;
	bool ShouldEnterClimb(const AClimbVolume* Volume) const;

	/** Input normalizado no plano horizontal (0..1). */
	FVector GetInputDirection2D() const;

	/** Desejo do jogador de correr; viaja no saved move. */
	uint8 bWantsToSprint : 1;

	/** Normal da última superfície escalada (para o impulso de topo). */
	FVector LastClimbNormal = FVector::ForwardVector;
};
