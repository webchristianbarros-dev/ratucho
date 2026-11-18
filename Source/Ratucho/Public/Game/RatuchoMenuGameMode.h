#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "RatuchoMenuGameMode.generated.h"

class SRatMainMenu;
class SWidget;

/** GameMode do mapa de menu (sem pawn; só UI). Mapeado pelo prefixo "L_MainMenu". */
UCLASS()
class RATUCHO_API ARatuchoMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARatuchoMenuGameMode();
};

/** Controller do menu principal: mostra a UI e executa as ações dos botões. */
UCLASS()
class RATUCHO_API ARatMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Hospeda uma partida (Listen Server) na Cozinha. */
	void StartMatch(int32 NumBots, int32 Weapon);

	/** Entra na partida de outro jogador na rede local. */
	void JoinMatch(const FString& Address, int32 Weapon);

	/** Alterna Baixo → Médio → Alto e aplica. Retorna o novo rótulo. */
	FText CycleGraphicsQuality();
	FText GetGraphicsQualityLabel() const;

	void QuitGame();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TSharedPtr<SRatMainMenu> Menu;
	TSharedPtr<SWidget> Container;
};
