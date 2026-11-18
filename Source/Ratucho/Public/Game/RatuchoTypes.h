#pragma once

#include "CoreMinimal.h"
#include "RatuchoTypes.generated.h"

UENUM(BlueprintType)
enum class ERatWeaponType : uint8
{
	Slingshot  UMETA(DisplayName = "Estilingue de Rolhas"),
	CapBlaster UMETA(DisplayName = "Lançador de Tampinhas"),
	Count      UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ERatMatchPhase : uint8
{
	Warmup,
	InProgress,
	Ended
};

UENUM(BlueprintType)
enum class ERatCheeseEvent : uint8
{
	Spawned,
	PickedUp,
	Thrown,
	DroppedOnDeath,
	DroppedOnLeave
};

/** Parâmetros de balanceamento de uma arma (todos ajustáveis sem tocar em lógica). */
USTRUCT(BlueprintType)
struct RATUCHO_API FRatWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") FText Description;
	/** Dano por projétil. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0")) float Damage = 34.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "100")) float ProjectileSpeed = 3600.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0")) float ProjectileGravityScale = 0.35f;
	/** Alcance máximo (cm); define o tempo de vida do projétil. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "100")) float MaxRange = 4500.f;
	/** Dispersão (meio-ângulo do cone, graus). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0")) float SpreadDegrees = 0.5f;
	/** Projéteis por gatilho (rajada). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "1")) int32 ShotsPerTrigger = 1;
	/** Intervalo entre tiros da mesma rajada (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0")) float BurstShotInterval = 0.07f;
	/** Intervalo mínimo entre gatilhos (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.02")) float FireInterval = 0.65f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "1")) int32 MagazineSize = 6;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0")) float ReloadTime = 1.6f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "1")) float ProjectileRadius = 9.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") FVector ProjectileScale = FVector(0.12f, 0.12f, 0.16f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") FLinearColor ProjectileColor = FLinearColor(0.6f, 0.4f, 0.2f);
	/** Alcance que os bots consideram "efetivo" para abrir fogo (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "100")) float BotEngageRange = 2600.f;
};

/** Perfil de personalidade de um bot. */
USTRUCT(BlueprintType)
struct RATUCHO_API FRatBotProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bot") FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bot") FString Trait;
	/** Vontade de procurar combate (0–1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bot", meta = (ClampMin = "0", ClampMax = "1")) float Aggression = 0.5f;
	/** Prioridade de ir atrás do queijo (0–1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bot", meta = (ClampMin = "0", ClampMax = "1")) float CheesePriority = 0.5f;
	/** Preferência por alvos enfraquecidos (0–1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bot", meta = (ClampMin = "0", ClampMax = "1")) float Opportunism = 0.3f;
	/** Tendência a flanquear ao perseguir (0–1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bot", meta = (ClampMin = "0", ClampMax = "1")) float FlankBias = 0.f;
	/** Fração da vida abaixo da qual foge (0–1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bot", meta = (ClampMin = "0", ClampMax = "1")) float CautionHealth = 0.25f;
	/** Chance de mudar de ideia aleatoriamente (0–1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bot", meta = (ClampMin = "0", ClampMax = "1")) float Unpredictability = 0.1f;
	/** Erro de mira base (graus). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bot", meta = (ClampMin = "0")) float AimErrorDegrees = 3.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bot") FLinearColor Color = FLinearColor::Gray;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bot") ERatWeaponType PreferredWeapon = ERatWeaponType::Slingshot;
};

namespace RatuchoDefaults
{
	RATUCHO_API FRatWeaponStats MakeWeaponStats(ERatWeaponType Type);
	RATUCHO_API TArray<FRatBotProfile> MakeBotRoster();
	RATUCHO_API TArray<FLinearColor> MakePlayerColors();

	/** Caminhos dos mapas. */
	inline const TCHAR* MenuMap = TEXT("/Game/Ratucho/Maps/L_MainMenu");
	inline const TCHAR* KitchenMap = TEXT("/Game/Ratucho/Maps/Levels/L_Cozinha");
}
