#pragma once

#include "CoreMinimal.h"
#include "Game/RatuchoTypes.h"
#include "GameFramework/GameStateBase.h"
#include "RatuchoGameState.generated.h"

class ACheese;
class APlayerState;
class ARatPlayerState;

/** Entrada local do feed de eventos (montada em cada máquina a partir de multicasts). */
struct FRatFeedEntry
{
	FString Text;
	FLinearColor Color = FLinearColor::White;
	double Time = 0.0;
};

/**
 * Estado público e replicado da partida: fase, cronômetro, queijo e vencedor.
 * Só o servidor escreve; HUD e bots apenas leem.
 */
UCLASS()
class RATUCHO_API ARatuchoGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ratucho")
	ERatMatchPhase Phase = ERatMatchPhase::Warmup;

	/** Instante (tempo de mundo do servidor) em que a fase atual termina. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ratucho")
	double PhaseEndServerTime = 0.0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ratucho")
	TObjectPtr<ACheese> Cheese = nullptr;

	/** Vencedor da rodada (nulo = ninguém venceu). Válido quando Phase == Ended. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ratucho")
	TObjectPtr<APlayerState> Winner = nullptr;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ratucho")
	int32 RoundNumber = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ratucho")
	float RoundDuration = 120.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ratucho")
	float RespawnDelay = 3.f;

	/** Local (não replicado): quem eliminou o jogador desta máquina por último. */
	FString LocalKilledBy;
	double LocalDeathTime = -100.0;

	UFUNCTION(BlueprintPure, Category = "Ratucho")
	float GetPhaseTimeRemaining() const;

	/** PlayerState de quem carrega o queijo (nulo se livre). */
	UFUNCTION(BlueprintPure, Category = "Ratucho")
	ARatPlayerState* GetCheeseCarrierState() const;

	// --- Feed (apresentação; o resultado oficial já está no estado replicado) ---
	UFUNCTION(NetMulticast, Reliable)
	void MulticastKillFeed(APlayerState* Killer, APlayerState* Victim);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastCheeseFeed(ERatCheeseEvent Event, APlayerState* Who);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAnnouncement(const FString& Text);

	const TArray<FRatFeedEntry>& GetFeed() const { return Feed; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void PushFeed(const FString& Text, const FLinearColor& Color);
	FString DescribeForLocal(const APlayerState* PS, bool bCapitalize) const;
	bool IsLocal(const APlayerState* PS) const;

	TArray<FRatFeedEntry> Feed;
};
