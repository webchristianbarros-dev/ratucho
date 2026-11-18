#include "Game/RatuchoMenuGameMode.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Game/RatuchoGameMode.h"
#include "Game/RatuchoTypes.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "RatuchoLog.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "UI/RatMenus.h"
#include "Widgets/SWeakWidget.h"

#define LOCTEXT_NAMESPACE "RatuchoMenu"

ARatuchoMenuGameMode::ARatuchoMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = ARatMenuPlayerController::StaticClass();
	HUDClass = nullptr;
}

void ARatMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	// Primeira execução: qualidade "Personalizado/auto" vira Médio (perfil seguro p/ GTX 1050 Ti).
	if (UGameUserSettings* GUS = GEngine->GetGameUserSettings())
	{
		if (GUS->GetOverallScalabilityLevel() < 0)
		{
			GUS->SetOverallScalabilityLevel(1);
			GUS->ApplySettings(false);
		}
	}
	const ARatuchoGameMode* Defaults = GetDefault<ARatuchoGameMode>();
	Menu = SNew(SRatMainMenu).Owner(this).MaxBots(Defaults->MaxBots).DefaultBots(Defaults->DefaultBots);
	Container = SNew(SWeakWidget).PossiblyNullContent(Menu);
	GEngine->GameViewport->AddViewportWidgetContent(Container.ToSharedRef(), 10);

	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(Menu);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	bShowMouseCursor = true;

	int32 ShotInterval = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("RatShots="), ShotInterval) && ShotInterval > 0)
	{
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([]()
		{
			FScreenshotRequest::RequestScreenshot(TEXT("RatShot_Menu"), true, false);
		}), static_cast<float>(ShotInterval), false);
	}
}

void ARatMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Container.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Container.ToSharedRef());
	}
	Container.Reset();
	Menu.Reset();
	Super::EndPlay(EndPlayReason);
}

void ARatMenuPlayerController::StartMatch(int32 NumBots, int32 Weapon)
{
	const FString Options = FString::Printf(TEXT("listen?Bots=%d?Weapon=%d"), NumBots, Weapon);
	UE_LOG(LogRatucho, Display, TEXT("Iniciando partida: %s %s"), RatuchoDefaults::KitchenMap, *Options);
	UGameplayStatics::OpenLevel(this, FName(RatuchoDefaults::KitchenMap), true, Options);
}

void ARatMenuPlayerController::JoinMatch(const FString& Address, int32 Weapon)
{
	FString Addr = Address.TrimStartAndEnd();
	if (Addr.IsEmpty())
	{
		Addr = TEXT("127.0.0.1");
	}
	const FString URL = FString::Printf(TEXT("%s?Weapon=%d"), *Addr, Weapon);
	UE_LOG(LogRatucho, Display, TEXT("Conectando em %s"), *URL);
	ClientTravel(URL, TRAVEL_Absolute);
}

FText ARatMenuPlayerController::GetGraphicsQualityLabel() const
{
	const UGameUserSettings* GUS = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	const int32 Level = GUS ? GUS->GetOverallScalabilityLevel() : 1;
	switch (Level)
	{
	case 0:  return LOCTEXT("Low", "Baixo");
	case 1:  return LOCTEXT("Medium", "Médio");
	case -1: return LOCTEXT("Custom", "Personalizado");
	default: return LOCTEXT("High", "Alto");
	}
}

FText ARatMenuPlayerController::CycleGraphicsQuality()
{
	if (UGameUserSettings* GUS = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		int32 Level = GUS->GetOverallScalabilityLevel();
		Level = (Level < 0 || Level >= 2) ? 0 : Level + 1; // Baixo(0) → Médio(1) → Alto(2) → Baixo
		GUS->SetOverallScalabilityLevel(Level);
		GUS->ApplySettings(false);
		UE_LOG(LogRatucho, Display, TEXT("Qualidade gráfica: %d"), Level);
	}
	return GetGraphicsQualityLabel();
}

void ARatMenuPlayerController::QuitGame()
{
	ConsoleCommand(TEXT("quit"));
}

#undef LOCTEXT_NAMESPACE
