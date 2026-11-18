#include "Game/RatuchoGameMode.h"

#include "AI/RatBotController.h"
#include "Characters/RatCharacter.h"
#include "Characters/RatMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/RatuchoGameState.h"
#include "Gameplay/Cheese.h"
#include "Kismet/GameplayStatics.h"
#include "Player/RatPlayerController.h"
#include "Player/RatPlayerState.h"
#include "RatuchoLog.h"
#include "TimerManager.h"
#include "UI/RatHUD.h"
#include "World/RatSpawnPoints.h"

ARatuchoGameMode::ARatuchoGameMode()
{
	DefaultPawnClass = ARatCharacter::StaticClass();
	PlayerControllerClass = ARatPlayerController::StaticClass();
	PlayerStateClass = ARatPlayerState::StaticClass();
	GameStateClass = ARatuchoGameState::StaticClass();
	HUDClass = ARatHUD::StaticClass();
	CheeseClass = ACheese::StaticClass();
	BotControllerClass = ARatBotController::StaticClass();
	bStartPlayersAsSpectators = false;
}

ARatuchoGameState* ARatuchoGameMode::GetRatGameState() const
{
	return GetGameState<ARatuchoGameState>();
}

bool ARatuchoGameMode::IsRoundInProgress() const
{
	const ARatuchoGameState* GS = GetRatGameState();
	return GS && GS->Phase == ERatMatchPhase::InProgress;
}

void ARatuchoGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	RequestedBots = FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("Bots"), DefaultBots), 0, MaxBots);
	HostWeapon = static_cast<ERatWeaponType>(FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("Weapon"), 0), 0, 1));
	AutoRestartDelay = FMath::Max(0, UGameplayStatics::GetIntOption(Options, TEXT("AutoRestart"), 0));
	bLogBots = UGameplayStatics::GetIntOption(Options, TEXT("BotLog"), 0) != 0;
	UE_LOG(LogRatucho, Display, TEXT("InitGame %s: Bots=%d (max %d) HostWeapon=%d"), *MapName, RequestedBots, MaxBots, static_cast<int32>(HostWeapon));
}

FString ARatuchoGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	const FString Result = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	if (ARatPlayerState* PS = NewPlayerController ? NewPlayerController->GetPlayerState<ARatPlayerState>() : nullptr)
	{
		const int32 Index = NextHumanIndex++;
		const int32 WeaponOpt = UGameplayStatics::GetIntOption(Options, TEXT("Weapon"), static_cast<int32>(HostWeapon));
		PS->Weapon = static_cast<ERatWeaponType>(FMath::Clamp(WeaponOpt, 0, 1));
		const TArray<FLinearColor> Colors = RatuchoDefaults::MakePlayerColors();
		PS->RatColor = Colors[(Index - 1) % Colors.Num()];

		// "Name" é preenchido pela engine com o nome do computador; usamos "RatName".
		FString Name = UGameplayStatics::ParseOption(Options, TEXT("RatName"));
		if (Name.IsEmpty())
		{
			Name = FString::Printf(TEXT("Jogador %d"), Index);
		}
		ChangeName(NewPlayerController, Name.Left(16), false);
		UE_LOG(LogRatucho, Display, TEXT("Novo jogador: %s Arma=%d"), *Name, WeaponOpt);
	}
	return Result;
}

void ARatuchoGameMode::StartPlay()
{
	Super::StartPlay();

	if (ARatuchoGameState* GS = GetRatGameState())
	{
		GS->RoundDuration = RoundDuration;
		GS->RespawnDelay = RespawnDelay;
		GS->RoundNumber = 1;
		GS->Phase = ERatMatchPhase::Warmup;
		GS->PhaseEndServerTime = GS->GetServerWorldTimeSeconds() + WarmupDuration;
	}
	SpawnBots();
	GetWorldTimerManager().SetTimer(MatchTimer, this, &ARatuchoGameMode::TickMatch, 0.1f, true);
}

void ARatuchoGameMode::TickMatch()
{
	ARatuchoGameState* GS = GetRatGameState();
	if (!GS)
	{
		return;
	}
	const float Remaining = GS->GetPhaseTimeRemaining();
	switch (GS->Phase)
	{
	case ERatMatchPhase::Warmup:
		if (Remaining <= 0.f)
		{
			StartRound();
		}
		break;
	case ERatMatchPhase::InProgress:
		if (Remaining <= 0.f)
		{
			EndRound();
		}
		break;
	case ERatMatchPhase::Ended:
		if (AutoRestartDelay > 0.f && GS->GetServerWorldTimeSeconds() - GS->PhaseEndServerTime >= AutoRestartDelay)
		{
			RestartRound();
		}
		break;
	default:
		break;
	}

	if (bLogBots && GetWorld()->GetTimeSeconds() - LastBotLogTime >= 5.0)
	{
		LastBotLogTime = GetWorld()->GetTimeSeconds();
		LogBots();
	}
}

void ARatuchoGameMode::LogBots() const
{
	static const TCHAR* GoalNames[] = { TEXT("Idle"), TEXT("GetCheese"), TEXT("ChaseCarrier"), TEXT("HoldCheese"), TEXT("Fight"), TEXT("Flee"), TEXT("Wander") };
	for (const TWeakObjectPtr<ARatBotController>& Weak : Bots)
	{
		const ARatBotController* Bot = Weak.Get();
		if (!Bot)
		{
			continue;
		}
		const ARatCharacter* Rat = Cast<ARatCharacter>(Bot->GetPawn());
		const int32 G = static_cast<int32>(Bot->GetGoal());
		UE_LOG(LogRatucho, Display, TEXT("[Bot] %-9s goal=%s pawn=%s loc=%s hp=%.0f move=%d"),
			*Bot->GetProfile().Name, G < 7 ? GoalNames[G] : TEXT("?"), Rat ? (Rat->IsDead() ? TEXT("morto") : TEXT("vivo")) : TEXT("SEM PAWN"),
			Rat ? *Rat->GetActorLocation().ToCompactString() : TEXT("-"), Rat ? Rat->GetHealth() : 0.f, static_cast<int32>(Bot->GetMoveStatus()));
	}
}

APawn* ARatuchoGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	// Pontos de spawn ficam no chão: sobe o pawn pela meia-altura da cápsula (+ folga).
	FTransform T = SpawnTransform;
	T.AddToTranslation(FVector(0.f, 0.f, 40.f));
	FActorSpawnParameters Params;
	Params.Instigator = GetInstigator();
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);
	return GetWorld()->SpawnActor<APawn>(PawnClass, T, Params);
}

void ARatuchoGameMode::StartRound()
{
	ARatuchoGameState* GS = GetRatGameState();
	GS->Phase = ERatMatchPhase::InProgress;
	GS->PhaseEndServerTime = GS->GetServerWorldTimeSeconds() + RoundDuration;
	SpawnCheese();
	GS->MulticastAnnouncement(TEXT("Valendo! Pegue o queijo e segure até o fim."));
	UE_LOG(LogRatucho, Display, TEXT("Rodada %d iniciada (%.0f s)"), GS->RoundNumber, RoundDuration);
}

void ARatuchoGameMode::EndRound()
{
	ARatuchoGameState* GS = GetRatGameState();
	GS->Phase = ERatMatchPhase::Ended;
	GS->PhaseEndServerTime = GS->GetServerWorldTimeSeconds();
	GS->Winner = GS->GetCheeseCarrierState();

	for (auto& Pair : RespawnTimers)
	{
		GetWorldTimerManager().ClearTimer(Pair.Value);
	}
	RespawnTimers.Reset();

	for (ARatCharacter* Rat : TActorRange<ARatCharacter>(GetWorld()))
	{
		Rat->ServerFreeze();
	}

	if (GS->Winner)
	{
		GS->MulticastAnnouncement(FString::Printf(TEXT("%s terminou com o queijo e venceu!"), *GS->Winner->GetPlayerName()));
	}
	else
	{
		GS->MulticastAnnouncement(TEXT("Ninguém venceu — o queijo ficou livre quando o tempo acabou."));
	}
	UE_LOG(LogRatucho, Display, TEXT("Rodada encerrada. Vencedor: %s"), GS->Winner ? *GS->Winner->GetPlayerName() : TEXT("ninguém"));
}

void ARatuchoGameMode::RestartRound()
{
	ARatuchoGameState* GS = GetRatGameState();
	if (!GS)
	{
		return;
	}
	for (auto& Pair : RespawnTimers)
	{
		GetWorldTimerManager().ClearTimer(Pair.Value);
	}
	RespawnTimers.Reset();

	// Remove o queijo ANTES dos ratos (assim nenhum portador "derruba" um queijo extra).
	if (GS->Cheese)
	{
		ACheese* Old = GS->Cheese;
		GS->Cheese = nullptr;
		Old->Destroy();
	}
	GS->Winner = nullptr;

	for (APlayerState* PS : GS->PlayerArray)
	{
		if (ARatPlayerState* RPS = Cast<ARatPlayerState>(PS))
		{
			RPS->ResetRoundStats();
		}
	}

	TArray<AController*> Controllers;
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		if (AController* C = It->Get())
		{
			Controllers.Add(C);
		}
	}
	for (AController* C : Controllers)
	{
		if (APawn* P = C->GetPawn())
		{
			C->UnPossess();
			P->Destroy();
		}
	}
	for (AController* C : Controllers)
	{
		if (C->PlayerState && (C->IsA<APlayerController>() || C->IsA<ARatBotController>()))
		{
			RestartPlayer(C);
		}
	}

	GS->RoundNumber++;
	GS->Phase = ERatMatchPhase::Warmup;
	GS->PhaseEndServerTime = GS->GetServerWorldTimeSeconds() + WarmupDuration;
	GS->MulticastAnnouncement(TEXT("Nova rodada! Prepare-se..."));
}

void ARatuchoGameMode::SpawnBots()
{
	TArray<FRatBotProfile> Roster = RatuchoDefaults::MakeBotRoster();
	// Embaralha para que cada partida tenha uma combinação diferente (nomes nunca se repetem).
	for (int32 i = Roster.Num() - 1; i > 0; --i)
	{
		Roster.Swap(i, FMath::RandRange(0, i));
	}
	const int32 Count = FMath::Min(RequestedBots, Roster.Num());
	for (int32 i = 0; i < Count; ++i)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ARatBotController* Bot = GetWorld()->SpawnActor<ARatBotController>(BotControllerClass, Params);
		if (!Bot)
		{
			continue;
		}
		const FRatBotProfile& Profile = Roster[i];
		Bot->SetProfile(Profile);
		if (ARatPlayerState* PS = Bot->GetPlayerState<ARatPlayerState>())
		{
			PS->SetPlayerName(Profile.Name);
			PS->SetIsABot(true);
			PS->RatColor = Profile.Color;
			PS->Weapon = Profile.PreferredWeapon;
			PS->Trait = Profile.Trait;
		}
		RestartPlayer(Bot);
		Bots.Add(Bot);
	}
	UE_LOG(LogRatucho, Display, TEXT("%d bots criados no servidor"), Count);
}

void ARatuchoGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
	Super::SetPlayerDefaults(PlayerPawn);
	ARatCharacter* Rat = Cast<ARatCharacter>(PlayerPawn);
	if (!Rat)
	{
		return;
	}
	ERatWeaponType WeaponType = HostWeapon;
	if (const ARatPlayerState* PS = Rat->GetPlayerState<ARatPlayerState>())
	{
		WeaponType = PS->Weapon;
	}
	Rat->ServerInitForRound(WeaponType, RespawnInvulnerability);
	if (Rat->IsBotControlled() && Rat->GetRatMovement())
	{
		Rat->GetRatMovement()->bUseRVOAvoidance = true;
		Rat->GetRatMovement()->AvoidanceConsiderationRadius = 300.f;
	}
}

AActor* ARatuchoGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (ARatSpawnPoint* Point = SelectSpawnPoint(Player))
	{
		return Point;
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}

ARatSpawnPoint* ARatuchoGameMode::SelectSpawnPoint(AController* For) const
{
	UWorld* World = GetWorld();
	TArray<ARatSpawnPoint*> Points;
	for (ARatSpawnPoint* P : TActorRange<ARatSpawnPoint>(World))
	{
		Points.Add(P);
	}
	if (Points.Num() == 0)
	{
		return nullptr;
	}

	TArray<const ARatCharacter*> Enemies;
	const ARatCharacter* Carrier = nullptr;
	for (const ARatCharacter* Rat : TActorRange<ARatCharacter>(World))
	{
		if (Rat && !Rat->IsDead() && Rat->GetController() != For)
		{
			Enemies.Add(Rat);
			if (Rat->IsCarryingCheese())
			{
				Carrier = Rat;
			}
		}
	}
	const ARatuchoGameState* GS = GetRatGameState();
	const ACheese* Cheese = GS ? GS->Cheese.Get() : nullptr;

	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(32.f, 36.f);
	ARatSpawnPoint* Best = nullptr;
	float BestScore = -MAX_flt;

	const double Now = World->GetTimeSeconds();
	for (ARatSpawnPoint* P : Points)
	{
		// Reservado: alguém nasceu aqui há pouco (spawns simultâneos).
		if (const double* Used = RecentSpawnUse.Find(P))
		{
			if (Now - *Used < 1.5)
			{
				continue;
			}
		}
		const FVector Loc = P->GetActorLocation() + FVector(0.f, 0.f, 40.f);

		// Dentro de objeto ou em cima de outro rato?
		FCollisionQueryParams Q(SCENE_QUERY_STAT(RatSpawnTest), false);
		if (World->OverlapBlockingTestByChannel(Loc, FQuat::Identity, ECC_Pawn, Capsule, Q))
		{
			continue;
		}
		// Precisa de chão logo abaixo (nada de nascer no ar).
		FHitResult Floor;
		if (!World->LineTraceSingleByChannel(Floor, Loc, Loc - FVector(0.f, 0.f, 150.f), ECC_Visibility, Q))
		{
			continue;
		}
		// Nunca em cima do queijo.
		if (Cheese && FVector::Dist(Cheese->GetActorLocation(), Loc) < 300.f)
		{
			continue;
		}

		float MinDist = 5000.f;
		int32 VisibleEnemies = 0;
		for (const ARatCharacter* E : Enemies)
		{
			const float D = FVector::Dist(E->GetActorLocation(), Loc);
			MinDist = FMath::Min(MinDist, D);
			if (D < 3500.f)
			{
				FCollisionQueryParams LOS(SCENE_QUERY_STAT(RatSpawnLOS), false, E);
				if (!World->LineTraceTestByChannel(E->GetPawnViewLocation(), Loc, ECC_Visibility, LOS))
				{
					++VisibleEnemies; // estaria na linha de tiro
				}
			}
		}
		float Score = FMath::Min(MinDist, 4000.f);
		if (MinDist < SpawnMinEnemyDistance)
		{
			Score -= 2000.f;
		}
		Score -= VisibleEnemies * 1200.f;
		if (Carrier)
		{
			Score += FMath::Min(FVector::Dist(Carrier->GetActorLocation(), Loc), 3000.f) * 0.3f;
		}
		Score += FMath::FRandRange(0.f, 700.f); // aleatoriedade entre pontos bons
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = P;
		}
	}
	ARatSpawnPoint* Chosen = Best ? Best : Points[FMath::RandRange(0, Points.Num() - 1)];
	RecentSpawnUse.Add(Chosen, Now);
	return Chosen;
}

void ARatuchoGameMode::SpawnCheese()
{
	ARatuchoGameState* GS = GetRatGameState();
	if (!GS || GS->Cheese)
	{
		return; // garante queijo único
	}
	TArray<ACheeseSpawnPoint*> Points;
	for (ACheeseSpawnPoint* P : TActorRange<ACheeseSpawnPoint>(GetWorld()))
	{
		Points.Add(P);
	}
	FVector Loc = FVector::ZeroVector;
	if (Points.Num() > 0)
	{
		Loc = Points[FMath::RandRange(0, Points.Num() - 1)]->GetActorLocation();
	}
	else if (ARatSpawnPoint* SP = SelectSpawnPoint(nullptr))
	{
		Loc = SP->GetActorLocation();
		UE_LOG(LogRatucho, Warning, TEXT("Mapa sem CheeseSpawnPoint; usando um ponto de rato."));
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ACheese* Cheese = GetWorld()->SpawnActor<ACheese>(CheeseClass, Loc + FVector(0.f, 0.f, 40.f), FRotator::ZeroRotator, Params);
	if (Cheese)
	{
		Cheese->PlaceOnGround(Loc + FVector(0.f, 0.f, 40.f));
		GS->Cheese = Cheese;
		NotifyCheeseEvent(ERatCheeseEvent::Spawned, nullptr);
		UE_LOG(LogRatucho, Display, TEXT("Queijo criado em %s"), *Cheese->GetActorLocation().ToCompactString());
	}
}

void ARatuchoGameMode::NotifyCheeseEvent(ERatCheeseEvent Event, APlayerState* Who)
{
	if (ARatuchoGameState* GS = GetRatGameState())
	{
		GS->MulticastCheeseFeed(Event, Who);
	}
}

void ARatuchoGameMode::OnRatKilled(ARatCharacter* Victim, AController* Killer)
{
	ARatuchoGameState* GS = GetRatGameState();
	AController* VictimController = Victim ? Victim->GetController() : nullptr;
	ARatPlayerState* VictimPS = Victim ? Victim->GetPlayerState<ARatPlayerState>() : nullptr;
	ARatPlayerState* KillerPS = (Killer && Killer != VictimController) ? Killer->GetPlayerState<ARatPlayerState>() : nullptr;

	if (VictimPS)
	{
		VictimPS->Deaths++;
	}
	if (KillerPS)
	{
		KillerPS->Kills++;
	}
	if (GS)
	{
		GS->MulticastKillFeed(KillerPS, VictimPS);
	}

	if (VictimController && GS && GS->Phase != ERatMatchPhase::Ended)
	{
		TWeakObjectPtr<AController> Weak(VictimController);
		FTimerHandle& Handle = RespawnTimers.FindOrAdd(Weak);
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Weak]()
		{
			if (Weak.IsValid())
			{
				Respawn(Weak.Get());
			}
		}), RespawnDelay, false);
	}
}

void ARatuchoGameMode::Respawn(AController* Controller)
{
	RespawnTimers.Remove(TWeakObjectPtr<AController>(Controller));
	const ARatuchoGameState* GS = GetRatGameState();
	if (!Controller || !GS || GS->Phase == ERatMatchPhase::Ended)
	{
		return;
	}
	if (APawn* Old = Controller->GetPawn())
	{
		Controller->UnPossess();
		Old->Destroy();
	}
	RestartPlayer(Controller);
}

void ARatuchoGameMode::Logout(AController* Exiting)
{
	if (FTimerHandle* Handle = RespawnTimers.Find(TWeakObjectPtr<AController>(Exiting)))
	{
		GetWorldTimerManager().ClearTimer(*Handle);
		RespawnTimers.Remove(TWeakObjectPtr<AController>(Exiting));
	}
	Super::Logout(Exiting);
}
