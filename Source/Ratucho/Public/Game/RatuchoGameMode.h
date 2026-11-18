#pragma once

#include "CoreMinimal.h"
#include "Game/RatuchoTypes.h"
#include "GameFramework/GameModeBase.h"
#include "RatuchoGameMode.generated.h"

class ACheese;
class ARatCharacter;
class ARatBotController;
class ARatuchoGameState;
class ARatSpawnPoint;

/**
 * Regras da partida — executa SOMENTE no servidor (host do Listen Server).
 *
 * Fluxo: Warmup (3 s, sem tiros/queijo) → InProgress (120 s) → Ended (resultado).
 * Decide: spawn do queijo, respawn seguro, eliminações, bots e vencedor.
 *
 * Opções de URL: ?listen ?Bots=N ?Weapon=0|1 ?Name=Fulano
 */
UCLASS(Config = Game)
class RATUCHO_API ARatuchoGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARatuchoGameMode();

	// --- Parâmetros (Config/DefaultGame.ini ou Blueprint) --------------------
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Ratucho|Round") float RoundDuration = 120.f;
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Ratucho|Round") float WarmupDuration = 3.f;
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Ratucho|Round") float RespawnDelay = 3.f;
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Ratucho|Round") float RespawnInvulnerability = 2.5f;
	/** Limite técnico inicial de bots (medido; ver docs/BALANCE.md). */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Ratucho|Bots") int32 MaxBots = 7;
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Ratucho|Bots") int32 DefaultBots = 5;
	/** Distância mínima desejada entre o ponto de respawn e inimigos vivos (cm). */
	UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Ratucho|Spawn") float SpawnMinEnemyDistance = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ratucho|Classes") TSubclassOf<ACheese> CheeseClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ratucho|Classes") TSubclassOf<ARatBotController> BotControllerClass;

	// --- API para outros sistemas --------------------------------------------
	bool IsRoundInProgress() const;
	void OnRatKilled(ARatCharacter* Victim, AController* Killer);
	void NotifyCheeseEvent(ERatCheeseEvent Event, APlayerState* Who);

	/** Reinicia a rodada no mesmo mapa (sem fechar o executável). */
	UFUNCTION(BlueprintCallable, Category = "Ratucho")
	void RestartRound();

	// --- AGameModeBase -------------------------------------------------------
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) override;
	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override { return false; }
	virtual void SetPlayerDefaults(APawn* PlayerPawn) override;
	virtual void Logout(AController* Exiting) override;
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

protected:
	void TickMatch();
	void StartRound();
	void EndRound();
	void SpawnBots();
	void SpawnCheese();
	void Respawn(AController* Controller);
	ARatSpawnPoint* SelectSpawnPoint(AController* For) const;
	ARatuchoGameState* GetRatGameState() const;

private:
	void LogBots() const;

	int32 RequestedBots = 5;
	/** Teste: reinicia sozinho N s após o fim da rodada (0 = desligado). URL ?AutoRestart=N */
	float AutoRestartDelay = 0.f;
	/** Teste: registra no log o estado dos bots a cada 5 s. URL ?BotLog=1 */
	bool bLogBots = false;
	double LastBotLogTime = 0.0;
	/** Pontos usados recentemente (evita dois ratos no mesmo ponto ao mesmo tempo). */
	mutable TMap<TWeakObjectPtr<ARatSpawnPoint>, double> RecentSpawnUse;
	ERatWeaponType HostWeapon = ERatWeaponType::Slingshot;
	int32 NextHumanIndex = 1;
	int32 LastCountdownSecond = -1;
	FTimerHandle MatchTimer;
	TMap<TWeakObjectPtr<AController>, FTimerHandle> RespawnTimers;
	TArray<TWeakObjectPtr<ARatBotController>> Bots;
};
