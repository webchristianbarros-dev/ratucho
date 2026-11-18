#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RatPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class SRatInGameMenu;
class SWidget;
struct FInputActionValue;

/** Ações de input (criadas em código; não dependem de assets). */
USTRUCT(BlueprintType)
struct FRatInputActions
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> Move = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> Look = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> Jump = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> Sprint = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> Fire = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> Interact = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> ThrowCheese = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> Reload = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> Scoreboard = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Input") TObjectPtr<UInputAction> Pause = nullptr;
};

/**
 * Controller do jogador humano: input local, UI local e pedidos ao servidor.
 * Não guarda estado global da partida (isso está no GameState/PlayerState).
 */
UCLASS()
class RATUCHO_API ARatPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARatPlayerController();

	const FRatInputActions& GetInputActions();

	bool IsScoreboardHeld() const { return bScoreboardHeld; }
	double GetLastHitConfirmTime() const { return LastHitConfirmTime; }
	bool WasLastHitAKill() const { return bLastHitWasKill; }

	/** Servidor → atirador: seu projétil acertou (hit marker). */
	UFUNCTION(Client, Unreliable)
	void ClientHitConfirmed(bool bKilled);

	/** Botão "Jogar novamente" (só o host pode reiniciar). */
	void RequestRestart();
	bool CanRestart() const;
	void ReturnToMainMenu();
	void QuitGame();
	void ClosePauseMenu();

	// --- Comandos de teste (console) ----------------------------------------
	/** 1 = anda/corre/pula sozinho; 2 = também atira e tenta pegar o queijo. */
	UFUNCTION(Exec) void RatAutoMove(int32 Mode);

	virtual void PlayerTick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;

	UFUNCTION(Server, Reliable)
	void ServerRequestRestart();

private:
	void EnsureInputCreated();
	void AddMappingContext();
	void Input_ScoreboardStarted(const FInputActionValue& Value);
	void Input_ScoreboardCompleted(const FInputActionValue& Value);
	void Input_Pause(const FInputActionValue& Value);
	void ShowMenu(bool bEndScreen);
	void HideMenu();
	void TickAutoMove(float DeltaTime);

	UPROPERTY(Transient) FRatInputActions Actions;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> GameplayContext;

	bool bScoreboardHeld = false;
	double LastHitConfirmTime = -100.0;
	bool bLastHitWasKill = false;

	TSharedPtr<SRatInGameMenu> MenuWidget;
	TSharedPtr<SWidget> MenuContainer;
	bool bMenuIsEndScreen = false;

	int32 AutoMoveMode = 0;
	float AutoMoveTime = 0.f;
	float AutoMoveLogTimer = 0.f;
};
