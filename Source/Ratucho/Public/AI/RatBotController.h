#pragma once

#include "AIController.h"
#include "CoreMinimal.h"
#include "Game/RatuchoTypes.h"
#include "RatBotController.generated.h"

class ARatCharacter;

UENUM()
enum class ERatBotGoal : uint8
{
	Idle,
	GetCheese,
	ChaseCarrier,
	HoldCheese,
	Fight,
	Flee,
	Wander
};

/**
 * IA de bot — existe SOMENTE no servidor. Usa o mesmo ARatCharacter, a mesma
 * arma (FireFromServer), as mesmas regras de queijo/dano/respawn do jogador.
 *
 * Percepção legítima:
 * - Inimigos: só quando visíveis (alcance + campo de visão + linha de visão),
 *   ou muito perto (audição), ou quando o atacaram; memória curta da última posição.
 * - Queijo: posição pública (o HUD de todo jogador mostra direção/distância do queijo).
 *
 * Máquina de estados simples e depurável (ver docs/BOTS.md). Comando de debug: RatBotDebug 1
 */
UCLASS()
class RATUCHO_API ARatBotController : public AAIController
{
	GENERATED_BODY()

public:
	ARatBotController();

	void SetProfile(const FRatBotProfile& InProfile) { Profile = InProfile; }
	const FRatBotProfile& GetProfile() const { return Profile; }
	ERatBotGoal GetGoal() const { return Goal; }

	/** Notificado quando o pawn leva dano (aprende quem atacou). */
	void NotifyDamagedBy(AController* Attacker);

	UPROPERTY(EditAnywhere, Category = "Ratucho|Bot") float ThinkInterval = 0.2f;
	UPROPERTY(EditAnywhere, Category = "Ratucho|Bot") float SightRange = 3800.f;
	UPROPERTY(EditAnywhere, Category = "Ratucho|Bot") float SightHalfAngle = 75.f;
	UPROPERTY(EditAnywhere, Category = "Ratucho|Bot") float HearingRange = 650.f;
	UPROPERTY(EditAnywhere, Category = "Ratucho|Bot") float MemorySeconds = 4.f;
	/** Tempo de reação antes do primeiro tiro num alvo recém-visto (s). */
	UPROPERTY(EditAnywhere, Category = "Ratucho|Bot") float ReactionTime = 0.35f;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	struct FEnemyMemory
	{
		TWeakObjectPtr<ARatCharacter> Rat;
		FVector LastKnown = FVector::ZeroVector;
		double LastSeen = -100.0;
		double FirstSeen = -100.0;
		bool bVisibleNow = false;
	};

	void Think();
	void UpdatePerception();
	ARatCharacter* PickTarget() const;
	void ChooseGoal(ARatCharacter* Target);
	void ExecuteGoal(ARatCharacter* Target);
	void HandleCombat(ARatCharacter* Target, float DeltaSeconds);
	void MoveTowards(const FVector& Dest, float Acceptance = 70.f);
	FVector PickWanderPoint() const;
	FVector PickFleePoint(const FVector& Threat) const;
	FVector PickCarrierHidePoint() const;
	ARatCharacter* GetRat() const;
	FEnemyMemory* FindMemory(const ARatCharacter* Rat);

	FRatBotProfile Profile;
	ERatBotGoal Goal = ERatBotGoal::Idle;
	TArray<FEnemyMemory> Memory;
	TWeakObjectPtr<ARatCharacter> CurrentTarget;

	double NextThinkTime = 0.0;
	double NextRandomSwitch = 0.0;
	double NextAimAdjust = 0.0;
	FVector CurrentMoveDest = FVector(MAX_flt);
	double LastMoveRequest = -100.0;
	FVector WanderDest = FVector::ZeroVector;
	double WanderUntil = 0.0;
	FVector AimOffset = FVector::ZeroVector;

	// Detecção de "preso"
	FVector LastProgressLocation = FVector::ZeroVector;
	double LastProgressTime = 0.0;
	double StrafeUntil = 0.0;
	float StrafeSign = 1.f;
	float FlankSign = 1.f;
};
