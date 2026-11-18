#pragma once

#include "CoreMinimal.h"
#include "Game/RatuchoTypes.h"
#include "GameFramework/PlayerState.h"
#include "RatPlayerState.generated.h"

/** Identidade e estatísticas públicas de um participante (jogador ou bot). Replicado a todos. */
UCLASS()
class RATUCHO_API ARatPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ratucho")
	int32 Kills = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ratucho")
	int32 Deaths = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ratucho")
	FLinearColor RatColor = FLinearColor::Gray;

	/** Arma escolhida; mantida entre respawns. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ratucho")
	ERatWeaponType Weapon = ERatWeaponType::Slingshot;

	/** Traço de personalidade (bots) — só informativo. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ratucho")
	FString Trait;

	void ResetRoundStats()
	{
		Kills = 0;
		Deaths = 0;
	}

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
