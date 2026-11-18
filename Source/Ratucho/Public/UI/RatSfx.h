#pragma once

#include "CoreMinimal.h"

/** Sons provisórios sintetizados em código (substituir por assets de áudio na Etapa 7). */
enum class ERatSfx : uint8
{
	FireSlingshot,
	FireCaps,
	HitWorld,
	HitRat,
	HitConfirm,
	KillConfirm,
	Death,
	CheeseMine,
	CheeseOther,
	Respawn,
	Countdown,
	RoundStart,
	RoundEnd,
	UiClick
};

namespace RatSfx
{
	/** Toca um efeito. Se Location for informado, é 3D; senão 2D. Nunca roda em servidor dedicado. */
	RATUCHO_API void PlayCue(const UObject* WorldContext, ERatSfx Cue, const FVector* Location = nullptr);
}
