#include "Game/RatuchoTypes.h"

#define LOCTEXT_NAMESPACE "Ratucho"

FRatWeaponStats RatuchoDefaults::MakeWeaponStats(ERatWeaponType Type)
{
	FRatWeaponStats S;
	switch (Type)
	{
	case ERatWeaponType::CapBlaster:
		// Rajada curta de 3 tampinhas: rápida, pouco dano por acerto, mais dispersão,
		// projétil reto e veloz. Forte de perto/média, fraca de longe.
		S.DisplayName = LOCTEXT("CapBlaster", "Lançador de Tampinhas");
		S.Description = LOCTEXT("CapBlasterDesc", "Rajada de 3 tampinhas. Rápido e dispersivo: ótimo de perto.");
		S.Damage = 11.f;
		S.ProjectileSpeed = 5200.f;
		S.ProjectileGravityScale = 0.08f;
		S.MaxRange = 2600.f;
		S.SpreadDegrees = 3.2f;
		S.ShotsPerTrigger = 3;
		S.BurstShotInterval = 0.075f;
		S.FireInterval = 0.5f;
		S.MagazineSize = 18;
		S.ReloadTime = 2.0f;
		S.ProjectileRadius = 7.f;
		S.ProjectileScale = FVector(0.13f, 0.13f, 0.04f);
		S.ProjectileColor = FLinearColor(0.9f, 0.15f, 0.12f);
		S.BotEngageRange = 1700.f;
		break;

	case ERatWeaponType::Slingshot:
	default:
		// Rolha pesada com arco: 3 acertos eliminam. Precisa, cadência moderada.
		S.DisplayName = LOCTEXT("Slingshot", "Estilingue de Rolhas");
		S.Description = LOCTEXT("SlingshotDesc", "Rolhas pesadas com arco. Preciso a média distância.");
		S.Damage = 34.f;
		S.ProjectileSpeed = 3600.f;
		S.ProjectileGravityScale = 0.35f;
		S.MaxRange = 4500.f;
		S.SpreadDegrees = 0.4f;
		S.ShotsPerTrigger = 1;
		S.BurstShotInterval = 0.f;
		S.FireInterval = 0.62f;
		S.MagazineSize = 6;
		S.ReloadTime = 1.6f;
		S.ProjectileRadius = 9.f;
		S.ProjectileScale = FVector(0.11f, 0.11f, 0.16f);
		S.ProjectileColor = FLinearColor(0.62f, 0.42f, 0.22f);
		S.BotEngageRange = 2800.f;
		break;
	}
	return S;
}

TArray<FRatBotProfile> RatuchoDefaults::MakeBotRoster()
{
	auto P = [](const TCHAR* Name, const TCHAR* Trait, float Aggr, float Cheese, float Opp, float Flank, float Caution, float Rand, float Aim, FLinearColor Color, ERatWeaponType W)
	{
		FRatBotProfile B;
		B.Name = Name; B.Trait = Trait; B.Aggression = Aggr; B.CheesePriority = Cheese; B.Opportunism = Opp;
		B.FlankBias = Flank; B.CautionHealth = Caution; B.Unpredictability = Rand; B.AimErrorDegrees = Aim;
		B.Color = Color; B.PreferredWeapon = W;
		return B;
	};

	using W = ERatWeaponType;
	return {
		P(TEXT("Pipoca"),   TEXT("agressivo"),     0.95f, 0.35f, 0.3f, 0.1f, 0.10f, 0.10f, 3.0f, FLinearColor(0.95f, 0.85f, 0.55f), W::CapBlaster),
		P(TEXT("Queijudo"), TEXT("caçador de queijo"), 0.35f, 1.0f, 0.2f, 0.0f, 0.20f, 0.05f, 4.0f, FLinearColor(0.85f, 0.6f, 0.2f), W::Slingshot),
		P(TEXT("Fubá"),     TEXT("oportunista"),   0.6f, 0.55f, 1.0f, 0.2f, 0.25f, 0.10f, 3.5f, FLinearColor(0.9f, 0.75f, 0.35f), W::Slingshot),
		P(TEXT("Bigode"),   TEXT("flanqueador"),   0.7f, 0.5f, 0.4f, 1.0f, 0.20f, 0.10f, 3.0f, FLinearColor(0.25f, 0.22f, 0.2f), W::CapBlaster),
		P(TEXT("Nina"),     TEXT("cautelosa"),     0.45f, 0.6f, 0.4f, 0.3f, 0.50f, 0.05f, 2.5f, FLinearColor(0.92f, 0.92f, 0.95f), W::Slingshot),
		P(TEXT("Parafuso"), TEXT("imprevisível"),  0.6f, 0.6f, 0.3f, 0.5f, 0.20f, 0.60f, 5.0f, FLinearColor(0.45f, 0.5f, 0.6f), W::CapBlaster),
		P(TEXT("Farofa"),   TEXT("equilibrado"),   0.55f, 0.55f, 0.5f, 0.3f, 0.30f, 0.15f, 3.5f, FLinearColor(0.7f, 0.45f, 0.3f), W::Slingshot),
		P(TEXT("Tampinha"), TEXT("veloz"),         0.65f, 0.7f, 0.3f, 0.6f, 0.30f, 0.20f, 4.0f, FLinearColor(0.55f, 0.35f, 0.5f), W::CapBlaster),
	};
}

TArray<FLinearColor> RatuchoDefaults::MakePlayerColors()
{
	return {
		FLinearColor(0.45f, 0.3f, 0.2f),   // marrom (jogador 1)
		FLinearColor(0.6f, 0.6f, 0.62f),   // cinza claro
		FLinearColor(0.2f, 0.2f, 0.25f),   // grafite
		FLinearColor(0.8f, 0.55f, 0.35f),  // caramelo
	};
}

#undef LOCTEXT_NAMESPACE
