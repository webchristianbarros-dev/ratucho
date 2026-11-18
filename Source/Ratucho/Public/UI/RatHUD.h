#pragma once

#include "CoreMinimal.h"
#include "Game/RatuchoTypes.h"
#include "GameFramework/HUD.h"
#include "RatHUD.generated.h"

class ARatCharacter;
class ARatuchoGameState;
class ARatPlayerState;
class UFont;

/**
 * HUD da partida desenhado em Canvas. Apenas EXIBE estado replicado
 * (GameState/PlayerState/personagem); nunca decide regras.
 */
UCLASS()
class RATUCHO_API ARatHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawTimerAndCheese(const ARatuchoGameState* GS, ARatCharacter* Me);
	void DrawPlayerStatus(ARatCharacter* Me);
	void DrawCrosshair(ARatCharacter* Me);
	void DrawNameplates(const ARatuchoGameState* GS, ARatCharacter* Me);
	void DrawCheeseMarker(const ARatuchoGameState* GS, ARatCharacter* Me);
	void DrawFeed(const ARatuchoGameState* GS);
	void DrawScoreboard(const ARatuchoGameState* GS, const FString& Title, float TopY);
	void DrawDeathOverlay(const ARatuchoGameState* GS, ARatCharacter* Me);
	void DrawEndScreen(const ARatuchoGameState* GS);
	void DrawHints(const ARatuchoGameState* GS, ARatCharacter* Me);
	void PlayPhaseSounds(const ARatuchoGameState* GS);

	void Text(const FString& Str, float X, float Y, const FLinearColor& Color, float Scale, bool bCenterX = false, UFont* Font = nullptr);
	void Panel(float X, float Y, float W, float H, const FLinearColor& Color);
	float S() const { return UIScale; }

	TArray<ARatPlayerState*> GetRanking(const ARatuchoGameState* GS) const;

	float UIScale = 1.f;
	ERatMatchPhase LastPhase = ERatMatchPhase::Warmup;
	int32 LastCountdown = -1;
	bool bPhaseInit = false;
};
