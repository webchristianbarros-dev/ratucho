#include "Player/RatPlayerController.h"

#include "Characters/RatCharacter.h"
#include "Characters/RatMovementComponent.h"
#include "Combat/RatWeaponComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Game/RatuchoGameMode.h"
#include "Game/RatuchoGameState.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "RatuchoLog.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "UI/RatMenus.h"
#include "UI/RatSfx.h"
#include "Widgets/SWeakWidget.h"

ARatPlayerController::ARatPlayerController()
{
}

void ARatPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		EnsureInputCreated();
		AddMappingContext();
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;

		// Teste visual: -RatShots=N tira 4 screenshots a cada N segundos (Saved/Screenshots).
		int32 ShotInterval = 0;
		if (FParse::Value(FCommandLine::Get(), TEXT("RatShots="), ShotInterval) && ShotInterval > 0)
		{
			FTimerHandle Handle;
			GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				static int32 Count = 0;
				if (Count++ < 4)
				{
					FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("RatShot_%d"), Count), false, false);
				}
			}), static_cast<float>(ShotInterval), true);
		}
	}
}

void ARatPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideMenu();
	Super::EndPlay(EndPlayReason);
}

const FRatInputActions& ARatPlayerController::GetInputActions()
{
	EnsureInputCreated();
	return Actions;
}

void ARatPlayerController::EnsureInputCreated()
{
	if (GameplayContext)
	{
		return;
	}
	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* IA = NewObject<UInputAction>(this, Name);
		IA->ValueType = Type;
		return IA;
	};
	Actions.Move        = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	Actions.Look        = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	Actions.Jump        = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	Actions.Sprint      = MakeAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	Actions.Fire        = MakeAction(TEXT("IA_Fire"), EInputActionValueType::Boolean);
	Actions.Interact    = MakeAction(TEXT("IA_Interact"), EInputActionValueType::Boolean);
	Actions.ThrowCheese = MakeAction(TEXT("IA_ThrowCheese"), EInputActionValueType::Boolean);
	Actions.Reload      = MakeAction(TEXT("IA_Reload"), EInputActionValueType::Boolean);
	Actions.Scoreboard  = MakeAction(TEXT("IA_Scoreboard"), EInputActionValueType::Boolean);
	Actions.Pause       = MakeAction(TEXT("IA_Pause"), EInputActionValueType::Boolean);

	GameplayContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Gameplay"));

	auto MapMove = [this](FKey Key, bool bSwizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& M = GameplayContext->MapKey(Actions.Move, Key);
		if (bSwizzle)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(GameplayContext);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			M.Modifiers.Add(Swizzle);
		}
		if (bNegate)
		{
			M.Modifiers.Add(NewObject<UInputModifierNegate>(GameplayContext));
		}
	};
	MapMove(EKeys::W, true, false);
	MapMove(EKeys::S, true, true);
	MapMove(EKeys::D, false, false);
	MapMove(EKeys::A, false, true);
	{
		FEnhancedActionKeyMapping& M = GameplayContext->MapKey(Actions.Look, EKeys::Mouse2D);
		UInputModifierNegate* NegY = NewObject<UInputModifierNegate>(GameplayContext);
		NegY->bX = false;
		NegY->bY = true;
		NegY->bZ = false;
		M.Modifiers.Add(NegY);
	}
	GameplayContext->MapKey(Actions.Jump, EKeys::SpaceBar);
	GameplayContext->MapKey(Actions.Sprint, EKeys::LeftShift);
	GameplayContext->MapKey(Actions.Fire, EKeys::LeftMouseButton);
	GameplayContext->MapKey(Actions.Interact, EKeys::E);
	GameplayContext->MapKey(Actions.ThrowCheese, EKeys::G);
	GameplayContext->MapKey(Actions.Reload, EKeys::R);
	GameplayContext->MapKey(Actions.Scoreboard, EKeys::Tab);
	GameplayContext->MapKey(Actions.Pause, EKeys::Escape);
}

void ARatPlayerController::AddMappingContext()
{
	if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Sub->RemoveMappingContext(GameplayContext);
		Sub->AddMappingContext(GameplayContext, 0);
	}
}

void ARatPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	EnsureInputCreated();
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Tab: SEGURAR para ver o ranking (fecha ao soltar).
		EIC->BindAction(Actions.Scoreboard, ETriggerEvent::Started, this, &ARatPlayerController::Input_ScoreboardStarted);
		EIC->BindAction(Actions.Scoreboard, ETriggerEvent::Completed, this, &ARatPlayerController::Input_ScoreboardCompleted);
		EIC->BindAction(Actions.Pause, ETriggerEvent::Started, this, &ARatPlayerController::Input_Pause);
	}
}

void ARatPlayerController::Input_ScoreboardStarted(const FInputActionValue&) { bScoreboardHeld = true; }
void ARatPlayerController::Input_ScoreboardCompleted(const FInputActionValue&) { bScoreboardHeld = false; }

void ARatPlayerController::Input_Pause(const FInputActionValue&)
{
	if (MenuWidget.IsValid())
	{
		if (!bMenuIsEndScreen)
		{
			HideMenu();
		}
		return;
	}
	ShowMenu(false);
}

void ARatPlayerController::ClientHitConfirmed_Implementation(bool bKilled)
{
	LastHitConfirmTime = GetWorld()->GetTimeSeconds();
	bLastHitWasKill = bKilled;
	RatSfx::PlayCue(this, bKilled ? ERatSfx::KillConfirm : ERatSfx::HitConfirm);
}

bool ARatPlayerController::CanRestart() const
{
	// Host do Listen Server = controller local que também é autoridade.
	return IsLocalController() && HasAuthority();
}

void ARatPlayerController::RequestRestart()
{
	ServerRequestRestart();
}

void ARatPlayerController::ServerRequestRestart_Implementation()
{
	if (!IsLocalController())
	{
		return; // cliente remoto não pode reiniciar a partida de todos
	}
	if (ARatuchoGameMode* GM = GetWorld()->GetAuthGameMode<ARatuchoGameMode>())
	{
		GM->RestartRound();
	}
}

void ARatPlayerController::ReturnToMainMenu()
{
	HideMenu();
	if (HasAuthority())
	{
		// Host: encerra a sessão para todos e volta ao menu.
		UGameplayStatics::OpenLevel(this, FName(RatuchoDefaults::MenuMap));
	}
	else
	{
		ConsoleCommand(TEXT("disconnect"));
	}
}

void ARatPlayerController::QuitGame()
{
	ConsoleCommand(TEXT("quit"));
}

void ARatPlayerController::ClosePauseMenu()
{
	if (!bMenuIsEndScreen)
	{
		HideMenu();
	}
}

void ARatPlayerController::ShowMenu(bool bEndScreen)
{
	if (!IsLocalController() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	HideMenu();
	bMenuIsEndScreen = bEndScreen;
	MenuWidget = SNew(SRatInGameMenu).Owner(this).bEndScreen(bEndScreen);
	MenuContainer = SNew(SWeakWidget).PossiblyNullContent(MenuWidget);
	GEngine->GameViewport->AddViewportWidgetContent(MenuContainer.ToSharedRef(), 20);

	FInputModeGameAndUI Mode;
	Mode.SetWidgetToFocus(MenuWidget);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
	bShowMouseCursor = true;
	SetIgnoreLookInput(true);
	if (ARatCharacter* Rat = Cast<ARatCharacter>(GetPawn()))
	{
		if (Rat->GetWeapon())
		{
			Rat->GetWeapon()->SetTriggerHeld(false);
		}
	}
}

void ARatPlayerController::HideMenu()
{
	if (MenuContainer.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MenuContainer.ToSharedRef());
	}
	const bool bHadMenu = MenuWidget.IsValid();
	MenuContainer.Reset();
	MenuWidget.Reset();
	if (bHadMenu && IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
		ResetIgnoreLookInput();
	}
}

void ARatPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!IsLocalController())
	{
		return;
	}

	// Tela de resultado aparece/desaparece conforme a fase replicada.
	if (const ARatuchoGameState* GS = GetWorld()->GetGameState<ARatuchoGameState>())
	{
		const bool bEnded = GS->Phase == ERatMatchPhase::Ended;
		if (bEnded && (!MenuWidget.IsValid() || !bMenuIsEndScreen))
		{
			ShowMenu(true);
		}
		else if (!bEnded && MenuWidget.IsValid() && bMenuIsEndScreen)
		{
			HideMenu();
		}
	}

	TickAutoMove(DeltaTime);
}

void ARatPlayerController::RatAutoMove(int32 Mode)
{
	AutoMoveMode = Mode;
	AutoMoveTime = 0.f;
	UE_LOG(LogRatucho, Display, TEXT("RatAutoMove %d"), Mode);
	if (Mode == 0)
	{
		if (ARatCharacter* Rat = Cast<ARatCharacter>(GetPawn()))
		{
			Rat->GetRatMovement()->SetSprinting(false);
			Rat->GetWeapon()->SetTriggerHeld(false);
		}
	}
}

void ARatPlayerController::TickAutoMove(float DeltaTime)
{
	ARatCharacter* Rat = Cast<ARatCharacter>(GetPawn());
	if (AutoMoveMode == 0 || !Rat || Rat->IsDead())
	{
		return;
	}
	AutoMoveTime += DeltaTime;
	const float Yaw = AutoMoveTime * 25.f;
	SetControlRotation(FRotator(-5.f, Yaw, 0.f));
	Rat->AddMovementInput(FRotator(0.f, Yaw, 0.f).Vector(), 1.f);
	Rat->GetRatMovement()->SetSprinting(FMath::Fmod(AutoMoveTime, 6.f) < 3.f);
	if (FMath::Fmod(AutoMoveTime, 2.f) < DeltaTime)
	{
		Rat->Jump();
	}
	else
	{
		Rat->StopJumping();
	}
	if (AutoMoveMode >= 2)
	{
		Rat->GetWeapon()->SetTriggerHeld(true);
	}

	AutoMoveLogTimer += DeltaTime;
	if (AutoMoveLogTimer >= 1.f)
	{
		AutoMoveLogTimer = 0.f;
		UE_LOG(LogRatucho, Display, TEXT("[AutoMove] %s Role=%d Loc=%s Speed=%.0f Sprint=%d HP=%.0f Ammo=%d"),
			*Rat->GetDisplayName(), static_cast<int32>(Rat->GetLocalRole()), *Rat->GetActorLocation().ToCompactString(),
			Rat->GetVelocity().Size2D(), Rat->GetRatMovement()->IsSprinting() ? 1 : 0, Rat->GetHealth(), Rat->GetWeapon()->GetAmmo());
	}
}
