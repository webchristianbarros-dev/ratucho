#include "UI/RatHUD.h"

#include "Camera/PlayerCameraManager.h"
#include "Characters/RatCharacter.h"
#include "Combat/RatWeaponComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "Game/RatuchoGameState.h"
#include "Gameplay/Cheese.h"
#include "Player/RatPlayerController.h"
#include "Player/RatPlayerState.h"
#include "UI/RatSfx.h"

namespace HudColors
{
	const FLinearColor Cheese(1.f, 0.82f, 0.15f);
	const FLinearColor PanelBg(0.05f, 0.04f, 0.03f, 0.55f);
	const FLinearColor White(1.f, 1.f, 1.f);
	const FLinearColor Dim(0.75f, 0.75f, 0.75f);
	const FLinearColor Health(0.35f, 0.9f, 0.35f);
	const FLinearColor HealthLow(0.95f, 0.3f, 0.25f);
	const FLinearColor Local(0.45f, 0.8f, 1.f);
}

static FString FormatTime(float Seconds)
{
	const int32 T = FMath::CeilToInt(FMath::Max(0.f, Seconds));
	return FString::Printf(TEXT("%d:%02d"), T / 60, T % 60);
}

void ARatHUD::Text(const FString& Str, float X, float Y, const FLinearColor& Color, float Scale, bool bCenterX, UFont* Font)
{
	if (!Canvas)
	{
		return;
	}
	UFont* F = Font ? Font : GEngine->GetLargeFont();
	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Str), F, Color);
	Item.Scale = FVector2D(Scale * UIScale, Scale * UIScale);
	Item.bOutlined = true;
	Item.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.85f);
	Item.bCentreX = bCenterX;
	Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.5f));
	Canvas->DrawItem(Item);
}

void ARatHUD::Panel(float X, float Y, float W, float H, const FLinearColor& Color)
{
	DrawRect(Color, X, Y, W, H);
}

TArray<ARatPlayerState*> ARatHUD::GetRanking(const ARatuchoGameState* GS) const
{
	TArray<ARatPlayerState*> Out;
	for (APlayerState* PS : GS->PlayerArray)
	{
		if (ARatPlayerState* R = Cast<ARatPlayerState>(PS))
		{
			Out.Add(R);
		}
	}
	Out.Sort([](const ARatPlayerState& A, const ARatPlayerState& B)
	{
		if (A.Kills != B.Kills) return A.Kills > B.Kills;
		if (A.Deaths != B.Deaths) return A.Deaths < B.Deaths;
		return A.GetPlayerName() < B.GetPlayerName();
	});
	return Out;
}

void ARatHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	// Fonte "Large" da engine é pequena; 1.45x deixa o HUD legível em 720p–1080p.
	UIScale = FMath::Clamp(Canvas->ClipY / 1080.f * 1.8f, 1.1f, 3.f);

	const ARatuchoGameState* GS = GetWorld()->GetGameState<ARatuchoGameState>();
	ARatPlayerController* PC = Cast<ARatPlayerController>(PlayerOwner);
	if (!GS || !PC)
	{
		return;
	}
	ARatCharacter* Me = Cast<ARatCharacter>(PC->GetPawn());

	PlayPhaseSounds(GS);

	if (GS->Phase == ERatMatchPhase::Ended)
	{
		DrawEndScreen(GS);
		return;
	}

	DrawNameplates(GS, Me);
	DrawCheeseMarker(GS, Me);
	DrawTimerAndCheese(GS, Me);
	DrawPlayerStatus(Me);
	DrawCrosshair(Me);
	DrawFeed(GS);
	DrawHints(GS, Me);
	DrawDeathOverlay(GS, Me);

	if (PC->IsScoreboardHeld())
	{
		DrawScoreboard(GS, TEXT("RANKING DA PARTIDA"), Canvas->ClipY * 0.18f);
	}
}

void ARatHUD::PlayPhaseSounds(const ARatuchoGameState* GS)
{
	if (!bPhaseInit)
	{
		bPhaseInit = true;
		LastPhase = GS->Phase;
	}
	if (GS->Phase != LastPhase)
	{
		if (GS->Phase == ERatMatchPhase::InProgress) RatSfx::PlayCue(this, ERatSfx::RoundStart);
		if (GS->Phase == ERatMatchPhase::Ended) RatSfx::PlayCue(this, ERatSfx::RoundEnd);
		LastPhase = GS->Phase;
	}
	if (GS->Phase == ERatMatchPhase::Warmup)
	{
		const int32 C = FMath::CeilToInt(GS->GetPhaseTimeRemaining());
		if (C != LastCountdown && C > 0)
		{
			RatSfx::PlayCue(this, ERatSfx::Countdown);
		}
		LastCountdown = C;
	}
	// Últimos 10 s da rodada: bipe a cada segundo.
	if (GS->Phase == ERatMatchPhase::InProgress)
	{
		const int32 C = FMath::CeilToInt(GS->GetPhaseTimeRemaining());
		if (C <= 10 && C != LastCountdown && C > 0)
		{
			RatSfx::PlayCue(this, ERatSfx::Countdown);
		}
		LastCountdown = C;
	}
}

void ARatHUD::DrawTimerAndCheese(const ARatuchoGameState* GS, ARatCharacter* Me)
{
	const float CX = Canvas->ClipX * 0.5f;
	const float Remaining = GS->GetPhaseTimeRemaining();

	Panel(CX - 120.f * S(), 14.f * S(), 240.f * S(), 66.f * S(), HudColors::PanelBg);
	if (GS->Phase == ERatMatchPhase::Warmup)
	{
		Text(FormatTime(GS->RoundDuration), CX, 16.f * S(), HudColors::White, 1.6f, true);
		Text(FString::Printf(TEXT("PREPARE-SE... %d"), FMath::CeilToInt(Remaining)), CX, Canvas->ClipY * 0.32f, HudColors::Cheese, 2.2f, true);
	}
	else
	{
		const FLinearColor TimeColor = Remaining <= 10.f ? HudColors::HealthLow : HudColors::White;
		Text(FormatTime(Remaining), CX, 16.f * S(), TimeColor, 1.6f, true);
	}

	// Estado do queijo.
	FString CheeseText;
	FLinearColor CheeseColor = HudColors::Cheese;
	const ACheese* Cheese = GS->Cheese;
	if (!Cheese)
	{
		CheeseText = GS->Phase == ERatMatchPhase::Warmup ? TEXT("O queijo aparece quando a rodada começar") : TEXT("Procurando o queijo...");
		CheeseColor = HudColors::Dim;
	}
	else if (Cheese->IsFree())
	{
		CheeseText = TEXT("QUEIJO LIVRE");
		if (Me && !Me->IsDead())
		{
			const float Dist = FVector::Dist(Me->GetActorLocation(), Cheese->GetActorLocation()) / 100.f;
			CheeseText += FString::Printf(TEXT("  •  %.0f m"), Dist);
		}
	}
	else if (Cheese->GetCarrier() == Me && Me)
	{
		CheeseText = TEXT("VOCÊ ESTÁ COM O QUEIJO!  Segure até o fim  (G = lançar)");
		CheeseColor = FLinearColor(0.5f, 1.f, 0.4f);
	}
	else
	{
		CheeseText = FString::Printf(TEXT("%s está com o queijo!"), *Cheese->GetCarrier()->GetDisplayName());
		CheeseColor = FLinearColor(1.f, 0.55f, 0.3f);
	}
	Text(CheeseText, CX, 86.f * S(), CheeseColor, 1.0f, true);
}

void ARatHUD::DrawPlayerStatus(ARatCharacter* Me)
{
	if (!Me)
	{
		return;
	}
	const float X = 30.f * S();
	const float Y = Canvas->ClipY - 110.f * S();
	const float W = 340.f * S();

	Panel(X - 10.f * S(), Y - 12.f * S(), W + 20.f * S(), 92.f * S(), HudColors::PanelBg);
	const float Frac = FMath::Clamp(Me->GetHealth() / Me->GetMaxHealth(), 0.f, 1.f);
	Text(FString::Printf(TEXT("VIDA  %.0f"), Me->GetHealth()), X, Y - 6.f * S(), HudColors::White, 0.9f);
	Panel(X, Y + 30.f * S(), W, 22.f * S(), FLinearColor(0.f, 0.f, 0.f, 0.6f));
	Panel(X, Y + 30.f * S(), W * Frac, 22.f * S(), Frac > 0.35f ? HudColors::Health : HudColors::HealthLow);
	if (Me->IsInvulnerable())
	{
		Text(TEXT("PROTEGIDO"), X + W - 110.f * S(), Y - 6.f * S(), HudColors::Local, 0.8f);
	}

	// Arma
	if (const URatWeaponComponent* Weapon = Me->GetWeapon())
	{
		const FRatWeaponStats& St = Weapon->GetStats();
		const float RX = Canvas->ClipX - 380.f * S();
		Panel(RX - 10.f * S(), Y - 12.f * S(), 370.f * S(), 92.f * S(), HudColors::PanelBg);
		Text(St.DisplayName.ToString(), RX, Y - 6.f * S(), HudColors::White, 0.9f);
		if (Weapon->IsReloading())
		{
			Text(TEXT("RECARREGANDO..."), RX, Y + 26.f * S(), HudColors::Cheese, 1.0f);
			Panel(RX, Y + 64.f * S(), 340.f * S() * Weapon->GetReloadProgress(), 6.f * S(), HudColors::Cheese);
		}
		else
		{
			Text(FString::Printf(TEXT("%d / %d"), Weapon->GetAmmo(), St.MagazineSize), RX, Y + 22.f * S(), Weapon->GetAmmo() > 0 ? HudColors::White : HudColors::HealthLow, 1.4f);
			Text(TEXT("R = recarregar"), RX + 190.f * S(), Y + 34.f * S(), HudColors::Dim, 0.7f);
		}
	}
}

void ARatHUD::DrawCrosshair(ARatCharacter* Me)
{
	if (!Me || Me->IsDead())
	{
		return;
	}
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;
	const float L = 9.f * S();
	const float G = 5.f * S();
	const FLinearColor C(1.f, 1.f, 1.f, 0.9f);
	Panel(CX - G - L, CY - 1.f, L, 2.f, C);
	Panel(CX + G, CY - 1.f, L, 2.f, C);
	Panel(CX - 1.f, CY - G - L, 2.f, L, C);
	Panel(CX - 1.f, CY + G, 2.f, L, C);

	// Hit marker
	const ARatPlayerController* PC = Cast<ARatPlayerController>(PlayerOwner);
	const double Since = GetWorld()->GetTimeSeconds() - PC->GetLastHitConfirmTime();
	if (Since < 0.25)
	{
		const FLinearColor HC = PC->WasLastHitAKill() ? FLinearColor(1.f, 0.25f, 0.2f) : FLinearColor(1.f, 1.f, 1.f);
		const float O = 10.f * S();
		const float Sz = 7.f * S();
		DrawLine(CX - O, CY - O, CX - O - Sz, CY - O - Sz, HC, 2.f);
		DrawLine(CX + O, CY - O, CX + O + Sz, CY - O - Sz, HC, 2.f);
		DrawLine(CX - O, CY + O, CX - O - Sz, CY + O + Sz, HC, 2.f);
		DrawLine(CX + O, CY + O, CX + O + Sz, CY + O + Sz, HC, 2.f);
	}
}

void ARatHUD::DrawNameplates(const ARatuchoGameState* GS, ARatCharacter* Me)
{
	if (!PlayerOwner || !PlayerOwner->PlayerCameraManager)
	{
		return;
	}
	const FVector CamLoc = PlayerOwner->PlayerCameraManager->GetCameraLocation();
	const FVector CamDir = PlayerOwner->PlayerCameraManager->GetCameraRotation().Vector();

	for (ARatCharacter* Rat : TActorRange<ARatCharacter>(GetWorld()))
	{
		if (!Rat || Rat == Me || Rat->IsDead())
		{
			continue;
		}
		const FVector Head = Rat->GetActorLocation() + FVector(0.f, 0.f, 70.f);
		const FVector To = Head - CamLoc;
		const float Dist = To.Size();
		if (Dist > 4500.f || FVector::DotProduct(To.GetSafeNormal(), CamDir) < 0.2f)
		{
			continue;
		}
		// Só mostra se houver linha de visão (não revela ratos atrás de paredes).
		FCollisionQueryParams Q(SCENE_QUERY_STAT(RatNameplate), false, Rat);
		if (Me)
		{
			Q.AddIgnoredActor(Me);
		}
		if (GetWorld()->LineTraceTestByChannel(CamLoc, Head, ECC_Visibility, Q))
		{
			continue;
		}
		const FVector Screen = Project(Head, false);
		const float Scale = FMath::Clamp(1.3f - Dist / 5000.f, 0.55f, 1.f);
		const bool bCarrier = Rat->IsCarryingCheese();
		const FLinearColor Color = bCarrier ? HudColors::Cheese : FLinearColor(1.f, 1.f, 1.f, 0.95f);
		FString Label = Rat->GetDisplayName();
		if (bCarrier)
		{
			Label += TEXT("  [QUEIJO]");
		}
		Text(Label, Screen.X, Screen.Y - 30.f * S(), Color, 0.75f * Scale, true);
		// Barrinha de vida discreta
		const float BW = 60.f * S() * Scale;
		const float Frac = FMath::Clamp(Rat->GetHealth() / Rat->GetMaxHealth(), 0.f, 1.f);
		Panel(Screen.X - BW * 0.5f, Screen.Y - 4.f * S(), BW, 4.f * S(), FLinearColor(0.f, 0.f, 0.f, 0.5f));
		Panel(Screen.X - BW * 0.5f, Screen.Y - 4.f * S(), BW * Frac, 4.f * S(), Frac > 0.35f ? HudColors::Health : HudColors::HealthLow);
	}
}

void ARatHUD::DrawCheeseMarker(const ARatuchoGameState* GS, ARatCharacter* Me)
{
	const ACheese* Cheese = GS->Cheese;
	if (!Cheese || !Cheese->IsFree() || !PlayerOwner || !PlayerOwner->PlayerCameraManager)
	{
		return;
	}
	const FVector Loc = Cheese->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector CamLoc = PlayerOwner->PlayerCameraManager->GetCameraLocation();
	const FRotator CamRot = PlayerOwner->PlayerCameraManager->GetCameraRotation();
	const FVector Local = CamRot.UnrotateVector(Loc - CamLoc);
	const float Dist = FVector::Dist(Loc, Me ? Me->GetActorLocation() : CamLoc) / 100.f;

	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;
	const FVector Screen = Project(Loc, false);
	const float Margin = 60.f * S();
	const bool bOnScreen = Local.X > 0.f && Screen.X > Margin && Screen.X < Canvas->ClipX - Margin && Screen.Y > Margin && Screen.Y < Canvas->ClipY - Margin;

	if (bOnScreen)
	{
		Text(FString::Printf(TEXT("QUEIJO %.0fm"), Dist), Screen.X, Screen.Y - 40.f * S(), HudColors::Cheese, 0.8f, true);
		Panel(Screen.X - 5.f * S(), Screen.Y - 10.f * S(), 10.f * S(), 10.f * S(), HudColors::Cheese);
	}
	else
	{
		// Seta na borda da tela apontando para o queijo.
		FVector2D Dir(Local.Y, -Local.Z);
		if (Local.X < 0.f && Dir.IsNearlyZero())
		{
			Dir = FVector2D(0.f, 1.f);
		}
		Dir.Normalize();
		const float R = FMath::Min(Canvas->ClipX, Canvas->ClipY) * 0.42f;
		const FVector2D P(CX + Dir.X * R, CY + Dir.Y * R);
		const FVector2D Tip = P + Dir * 18.f * S();
		const FVector2D Perp(-Dir.Y, Dir.X);
		DrawLine(Tip.X, Tip.Y, P.X + Perp.X * 12.f * S(), P.Y + Perp.Y * 12.f * S(), HudColors::Cheese, 3.f);
		DrawLine(Tip.X, Tip.Y, P.X - Perp.X * 12.f * S(), P.Y - Perp.Y * 12.f * S(), HudColors::Cheese, 3.f);
		Text(FString::Printf(TEXT("QUEIJO %.0fm"), Dist), P.X, P.Y + 14.f * S(), HudColors::Cheese, 0.7f, true);
	}
}

void ARatHUD::DrawFeed(const ARatuchoGameState* GS)
{
	const double Now = GetWorld()->GetTimeSeconds();
	const TArray<FRatFeedEntry>& Feed = GS->GetFeed();
	float Y = 20.f * S();
	const float X = Canvas->ClipX - 30.f * S();
	int32 Shown = 0;
	for (int32 i = Feed.Num() - 1; i >= 0 && Shown < 6; --i)
	{
		const double Age = Now - Feed[i].Time;
		if (Age > 6.0)
		{
			continue;
		}
		const float Alpha = Age > 5.0 ? static_cast<float>(6.0 - Age) : 1.f;
		FLinearColor C = Feed[i].Color;
		C.A = Alpha;
		float TW = 0.f, TH = 0.f;
		GetTextSize(Feed[i].Text, TW, TH, GEngine->GetLargeFont(), 0.8f * S());
		Panel(X - TW - 12.f * S(), Y - 2.f * S(), TW + 20.f * S(), TH + 4.f * S(), FLinearColor(0.f, 0.f, 0.f, 0.45f * Alpha));
		Text(Feed[i].Text, X - TW - 4.f * S(), Y, C, 0.8f);
		Y += TH + 8.f * S();
		++Shown;
	}
}

void ARatHUD::DrawHints(const ARatuchoGameState* GS, ARatCharacter* Me)
{
	if (!Me || Me->IsDead() || !GS->Cheese)
	{
		return;
	}
	const ACheese* Cheese = GS->Cheese;
	if (Cheese->IsFree() && FVector::Dist(Me->GetActorLocation(), Cheese->GetActorLocation()) <= Cheese->PickupRange)
	{
		Text(TEXT("[E]  Pegar o queijo"), Canvas->ClipX * 0.5f, Canvas->ClipY * 0.62f, HudColors::Cheese, 1.2f, true);
	}
	Text(TEXT("TAB = ranking   •   ESC = menu"), 30.f * S(), Canvas->ClipY - 150.f * S(), HudColors::Dim, 0.65f);
}

void ARatHUD::DrawDeathOverlay(const ARatuchoGameState* GS, ARatCharacter* Me)
{
	if (Me && !Me->IsDead())
	{
		return;
	}
	Panel(0.f, 0.f, Canvas->ClipX, Canvas->ClipY, FLinearColor(0.3f, 0.f, 0.f, 0.25f));
	const FString Who = GS->LocalKilledBy.IsEmpty() ? TEXT("Você foi eliminado!") : FString::Printf(TEXT("%s eliminou você!"), *GS->LocalKilledBy);
	Text(Who, Canvas->ClipX * 0.5f, Canvas->ClipY * 0.38f, FLinearColor(1.f, 0.5f, 0.45f), 1.8f, true);
	const float Left = FMath::Max(0.f, GS->RespawnDelay - static_cast<float>(GetWorld()->GetTimeSeconds() - GS->LocalDeathTime));
	Text(FString::Printf(TEXT("Reaparecendo em %.1f s"), Left), Canvas->ClipX * 0.5f, Canvas->ClipY * 0.46f, HudColors::White, 1.0f, true);
}

void ARatHUD::DrawScoreboard(const ARatuchoGameState* GS, const FString& Title, float TopY)
{
	const TArray<ARatPlayerState*> Ranking = GetRanking(GS);
	const ARatPlayerState* Carrier = GS->GetCheeseCarrierState();
	const APlayerState* LocalPS = PlayerOwner ? PlayerOwner->PlayerState : nullptr;

	const float W = 760.f * S();
	const float RowH = 40.f * S();
	const float X = Canvas->ClipX * 0.5f - W * 0.5f;
	const float H = (Ranking.Num() + 2) * RowH + 20.f * S();
	Panel(X, TopY, W, H, FLinearColor(0.03f, 0.03f, 0.04f, 0.82f));
	Text(Title, Canvas->ClipX * 0.5f, TopY + 8.f * S(), HudColors::Cheese, 1.1f, true);

	float Y = TopY + RowH + 6.f * S();
	Text(TEXT("#"), X + 20.f * S(), Y, HudColors::Dim, 0.75f);
	Text(TEXT("RATO"), X + 70.f * S(), Y, HudColors::Dim, 0.75f);
	Text(TEXT("ELIMINAÇÕES"), X + 360.f * S(), Y, HudColors::Dim, 0.75f);
	Text(TEXT("MORTES"), X + 500.f * S(), Y, HudColors::Dim, 0.75f);
	Text(TEXT("QUEIJO"), X + 620.f * S(), Y, HudColors::Dim, 0.75f);
	Y += RowH * 0.8f;

	for (int32 i = 0; i < Ranking.Num(); ++i)
	{
		const ARatPlayerState* PS = Ranking[i];
		const bool bLocal = PS == LocalPS;
		const bool bCarrier = PS == Carrier;
		if (bCarrier)
		{
			Panel(X + 6.f * S(), Y - 2.f * S(), W - 12.f * S(), RowH - 4.f * S(), FLinearColor(1.f, 0.8f, 0.1f, 0.25f));
		}
		else if (bLocal)
		{
			Panel(X + 6.f * S(), Y - 2.f * S(), W - 12.f * S(), RowH - 4.f * S(), FLinearColor(0.3f, 0.6f, 1.f, 0.2f));
		}
		const FLinearColor C = bCarrier ? HudColors::Cheese : (bLocal ? HudColors::Local : HudColors::White);
		FString Name = PS->GetPlayerName();
		if (bLocal) Name += TEXT(" (você)");
		if (PS->IsABot()) Name += TEXT("  (bot)");
		Text(FString::Printf(TEXT("%d"), i + 1), X + 20.f * S(), Y, C, 0.85f);
		Text(Name, X + 70.f * S(), Y, C, 0.85f);
		Text(FString::Printf(TEXT("%d"), PS->Kills), X + 400.f * S(), Y, C, 0.85f);
		Text(FString::Printf(TEXT("%d"), PS->Deaths), X + 530.f * S(), Y, C, 0.85f);
		Text(bCarrier ? TEXT("COM O QUEIJO") : TEXT("sem queijo"), X + 620.f * S(), Y, bCarrier ? HudColors::Cheese : HudColors::Dim, 0.75f);
		Y += RowH;
	}
	Text(TEXT("Eliminações não decidem a vitória: vence quem estiver com o queijo no fim."), Canvas->ClipX * 0.5f, TopY + H + 8.f * S(), HudColors::Dim, 0.7f, true);
}

void ARatHUD::DrawEndScreen(const ARatuchoGameState* GS)
{
	Panel(0.f, 0.f, Canvas->ClipX, Canvas->ClipY, FLinearColor(0.f, 0.f, 0.f, 0.45f));
	const float CX = Canvas->ClipX * 0.5f;
	const APlayerState* LocalPS = PlayerOwner ? PlayerOwner->PlayerState : nullptr;
	if (GS->Winner)
	{
		const bool bMe = GS->Winner == LocalPS;
		Text(bMe ? TEXT("VOCÊ VENCEU!") : FString::Printf(TEXT("%s VENCEU!"), *GS->Winner->GetPlayerName().ToUpper()), CX, Canvas->ClipY * 0.06f, HudColors::Cheese, 2.6f, true);
		Text(TEXT("Terminou a rodada com o queijo."), CX, Canvas->ClipY * 0.06f + 70.f * S(), HudColors::White, 1.0f, true);
	}
	else
	{
		Text(TEXT("NINGUÉM VENCEU"), CX, Canvas->ClipY * 0.06f, FLinearColor(1.f, 0.5f, 0.4f), 2.6f, true);
		Text(TEXT("O queijo ficou livre quando o tempo acabou — todos perderam."), CX, Canvas->ClipY * 0.06f + 70.f * S(), HudColors::White, 1.0f, true);
	}
	DrawScoreboard(GS, TEXT("RANKING FINAL"), Canvas->ClipY * 0.2f);
}
