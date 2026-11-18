#include "AI/RatBotController.h"

#include "Characters/RatCharacter.h"
#include "Characters/RatMovementComponent.h"
#include "Combat/RatWeaponComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/RatuchoGameState.h"
#include "Gameplay/Cheese.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "RatuchoLog.h"
#include "World/RatSpawnPoints.h"

ARatBotController::ARatBotController()
{
	bWantsPlayerState = true;
	PrimaryActorTick.bCanEverTick = true;
	bSetControlRotationFromPawnOrientation = false;
}

ARatCharacter* ARatBotController::GetRat() const
{
	return Cast<ARatCharacter>(GetPawn());
}

void ARatBotController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	Memory.Reset();
	CurrentTarget.Reset();
	Goal = ERatBotGoal::Idle;
	CurrentMoveDest = FVector(MAX_flt);
	LastProgressLocation = InPawn ? InPawn->GetActorLocation() : FVector::ZeroVector;
	LastProgressTime = GetWorld()->GetTimeSeconds();
	FlankSign = FMath::RandBool() ? 1.f : -1.f;
	NextThinkTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(0.f, ThinkInterval);
}

void ARatBotController::OnUnPossess()
{
	ClearFocus(EAIFocusPriority::Gameplay);
	Super::OnUnPossess();
}

void ARatBotController::NotifyDamagedBy(AController* Attacker)
{
	ARatCharacter* AttackerRat = Attacker ? Cast<ARatCharacter>(Attacker->GetPawn()) : nullptr;
	if (!AttackerRat || AttackerRat == GetRat())
	{
		return;
	}
	FEnemyMemory* M = FindMemory(AttackerRat);
	if (!M)
	{
		Memory.AddDefaulted();
		M = &Memory.Last();
		M->Rat = AttackerRat;
		M->FirstSeen = GetWorld()->GetTimeSeconds();
	}
	M->LastKnown = AttackerRat->GetActorLocation(); // sabe de onde veio o tiro
	M->LastSeen = GetWorld()->GetTimeSeconds();
}

ARatBotController::FEnemyMemory* ARatBotController::FindMemory(const ARatCharacter* Rat)
{
	return Memory.FindByPredicate([Rat](const FEnemyMemory& M) { return M.Rat.Get() == Rat; });
}

void ARatBotController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ARatCharacter* Rat = GetRat();
	const ARatuchoGameState* GS = GetWorld()->GetGameState<ARatuchoGameState>();
	if (!Rat || Rat->IsDead() || !GS || GS->Phase == ERatMatchPhase::Ended)
	{
		if (Rat)
		{
			Rat->SetCombatFacing(false);
		}
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now >= NextThinkTime)
	{
		NextThinkTime = Now + ThinkInterval;
		Think();
	}
	HandleCombat(CurrentTarget.Get(), DeltaSeconds);
}

void ARatBotController::Think()
{
	UpdatePerception();
	ARatCharacter* Target = PickTarget();
	CurrentTarget = Target;
	ChooseGoal(Target);
	ExecuteGoal(Target);

	// Preso? (sem progresso por 1,2 s enquanto tenta andar) -> pula e muda o rumo.
	ARatCharacter* Rat = GetRat();
	const double Now = GetWorld()->GetTimeSeconds();
	if (FVector::DistSquared(Rat->GetActorLocation(), LastProgressLocation) > FMath::Square(60.f))
	{
		LastProgressLocation = Rat->GetActorLocation();
		LastProgressTime = Now;
	}
	else if (Now - LastProgressTime > 1.2 && GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		Rat->Jump();
		LastProgressTime = Now;
		WanderUntil = 0.0;
		CurrentMoveDest = FVector(MAX_flt);
		if (Goal == ERatBotGoal::Wander)
		{
			WanderDest = PickWanderPoint();
		}
	}
}

void ARatBotController::UpdatePerception()
{
	ARatCharacter* Self = GetRat();
	const double Now = GetWorld()->GetTimeSeconds();
	const FVector Eye = Self->GetPawnViewLocation();
	const FVector Fwd = Self->GetActorForwardVector();
	const float CosFov = FMath::Cos(FMath::DegreesToRadians(SightHalfAngle));

	for (FEnemyMemory& M : Memory)
	{
		M.bVisibleNow = false;
	}

	for (ARatCharacter* Other : TActorRange<ARatCharacter>(GetWorld()))
	{
		if (!Other || Other == Self || Other->IsDead())
		{
			continue;
		}
		const FVector ToOther = Other->GetActorLocation() - Self->GetActorLocation();
		const float Dist = ToOther.Size();
		bool bPerceived = false;
		if (Dist <= HearingRange)
		{
			bPerceived = true; // muito perto: ouve/sente
		}
		else if (Dist <= SightRange && FVector::DotProduct(ToOther.GetSafeNormal(), Fwd) >= CosFov)
		{
			bPerceived = LineOfSightTo(Other, Eye);
		}
		if (!bPerceived)
		{
			continue;
		}
		FEnemyMemory* M = FindMemory(Other);
		if (!M)
		{
			Memory.AddDefaulted();
			M = &Memory.Last();
			M->Rat = Other;
			M->FirstSeen = Now;
		}
		else if (Now - M->LastSeen > 1.0)
		{
			M->FirstSeen = Now; // reapareceu: novo tempo de reação
		}
		M->bVisibleNow = true;
		M->LastSeen = Now;
		M->LastKnown = Other->GetActorLocation();
	}

	Memory.RemoveAll([this, Now](const FEnemyMemory& M)
	{
		return !M.Rat.IsValid() || M.Rat->IsDead() || Now - M.LastSeen > MemorySeconds;
	});
}

ARatCharacter* ARatBotController::PickTarget() const
{
	const ARatCharacter* Self = GetRat();
	ARatCharacter* Best = nullptr;
	float BestScore = -MAX_flt;
	for (const FEnemyMemory& M : Memory)
	{
		ARatCharacter* R = M.Rat.Get();
		if (!R || !M.bVisibleNow)
		{
			continue;
		}
		const float Dist = FVector::Dist(R->GetActorLocation(), Self->GetActorLocation());
		float Score = -Dist / 100.f;
		Score += (1.f - R->GetHealth() / R->GetMaxHealth()) * 40.f * Profile.Opportunism; // alvo fraco
		if (R->IsCarryingCheese())
		{
			Score += 30.f + 30.f * Profile.CheesePriority; // portador é alvo prioritário
		}
		if (R == CurrentTarget.Get())
		{
			Score += 8.f; // evita trocar de alvo a toda hora
		}
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = R;
		}
	}
	return Best;
}

void ARatBotController::ChooseGoal(ARatCharacter* Target)
{
	ARatCharacter* Self = GetRat();
	const ARatuchoGameState* GS = GetWorld()->GetGameState<ARatuchoGameState>();
	const ACheese* Cheese = GS ? GS->Cheese.Get() : nullptr;
	const double Now = GetWorld()->GetTimeSeconds();
	const float HealthFrac = Self->GetHealth() / Self->GetMaxHealth();

	if (Self->IsCarryingCheese())
	{
		Goal = ERatBotGoal::HoldCheese;
		return;
	}
	if (Target && HealthFrac <= Profile.CautionHealth)
	{
		Goal = ERatBotGoal::Flee;
		return;
	}

	// "Parafuso": às vezes muda de ideia sem motivo.
	if (Now >= NextRandomSwitch)
	{
		NextRandomSwitch = Now + FMath::FRandRange(2.f, 5.f);
		if (FMath::FRand() < Profile.Unpredictability)
		{
			Goal = FMath::RandBool() ? ERatBotGoal::Fight : ERatBotGoal::GetCheese;
			return;
		}
	}

	if (Cheese && Cheese->IsFree())
	{
		const float DistCheese = FVector::Dist(Cheese->GetActorLocation(), Self->GetActorLocation());
		// Inimigo visível e bot agressivo: briga antes, a menos que o queijo esteja muito perto.
		if (Target && Profile.Aggression > Profile.CheesePriority + 0.25f && DistCheese > 800.f)
		{
			Goal = ERatBotGoal::Fight;
		}
		else
		{
			Goal = ERatBotGoal::GetCheese;
		}
		return;
	}
	if (Cheese && Cheese->GetCarrier() && Cheese->GetCarrier() != Self)
	{
		Goal = (Target && Target != Cheese->GetCarrier() && Profile.Aggression > 0.8f) ? ERatBotGoal::Fight : ERatBotGoal::ChaseCarrier;
		return;
	}
	Goal = Target ? ERatBotGoal::Fight : ERatBotGoal::Wander;
}

void ARatBotController::ExecuteGoal(ARatCharacter* Target)
{
	ARatCharacter* Self = GetRat();
	const ARatuchoGameState* GS = GetWorld()->GetGameState<ARatuchoGameState>();
	const ACheese* Cheese = GS ? GS->Cheese.Get() : nullptr;
	const double Now = GetWorld()->GetTimeSeconds();

	switch (Goal)
	{
	case ERatBotGoal::GetCheese:
		if (Cheese)
		{
			MoveTowards(Cheese->GetActorLocation(), 40.f);
		}
		break;

	case ERatBotGoal::ChaseCarrier:
		if (Cheese && Cheese->GetCarrier())
		{
			const ARatCharacter* Carrier = Cheese->GetCarrier();
			// Intercepta: mira à frente do portador; flanqueadores abrem pelo lado.
			FVector Dest = Carrier->GetActorLocation() + Carrier->GetVelocity() * 0.6f;
			if (Profile.FlankBias > 0.2f)
			{
				const FVector ToCarrier = (Dest - Self->GetActorLocation()).GetSafeNormal2D();
				const FVector Side = FVector::CrossProduct(ToCarrier, FVector::UpVector) * FlankSign;
				const float Dist = FVector::Dist2D(Dest, Self->GetActorLocation());
				Dest += Side * FMath::Min(600.f, Dist * 0.5f) * Profile.FlankBias;
			}
			MoveTowards(Dest, 120.f);
		}
		break;

	case ERatBotGoal::HoldCheese:
		// Portador: continua se movendo para longe das ameaças conhecidas.
		if (Now > WanderUntil || FVector::Dist2D(Self->GetActorLocation(), WanderDest) < 150.f)
		{
			WanderDest = PickCarrierHidePoint();
			WanderUntil = Now + FMath::FRandRange(3.f, 6.f);
		}
		MoveTowards(WanderDest, 100.f);
		break;

	case ERatBotGoal::Fight:
		if (Target)
		{
			const FVector ToTarget = Target->GetActorLocation() - Self->GetActorLocation();
			const float Dist = ToTarget.Size();
			const float Ideal = Self->GetWeapon()->GetStats().BotEngageRange * 0.55f;
			FVector Dest = Target->GetActorLocation();
			if (Dist < Ideal)
			{
				// Já está perto: anda de lado (strafe) para não ser alvo fácil.
				if (Now > StrafeUntil)
				{
					StrafeSign = FMath::RandBool() ? 1.f : -1.f;
					StrafeUntil = Now + FMath::FRandRange(0.8f, 1.6f);
				}
				const FVector Side = FVector::CrossProduct(ToTarget.GetSafeNormal2D(), FVector::UpVector) * StrafeSign;
				Dest = Self->GetActorLocation() + Side * 300.f - ToTarget.GetSafeNormal2D() * 80.f;
			}
			else if (Profile.FlankBias > 0.2f)
			{
				const FVector Side = FVector::CrossProduct(ToTarget.GetSafeNormal2D(), FVector::UpVector) * FlankSign;
				Dest += Side * 500.f * Profile.FlankBias;
			}
			MoveTowards(Dest, 80.f);
		}
		else
		{
			// Sem alvo visível: vai até a última posição lembrada.
			const FEnemyMemory* Latest = nullptr;
			for (const FEnemyMemory& M : Memory)
			{
				if (!Latest || M.LastSeen > Latest->LastSeen)
				{
					Latest = &M;
				}
			}
			if (Latest)
			{
				MoveTowards(Latest->LastKnown, 100.f);
			}
			else
			{
				Goal = ERatBotGoal::Wander;
			}
		}
		break;

	case ERatBotGoal::Flee:
		if (Target)
		{
			if (Now > WanderUntil)
			{
				WanderDest = PickFleePoint(Target->GetActorLocation());
				WanderUntil = Now + 2.5f;
			}
			MoveTowards(WanderDest, 100.f);
		}
		break;

	case ERatBotGoal::Wander:
	default:
		if (Now > WanderUntil || FVector::Dist2D(Self->GetActorLocation(), WanderDest) < 150.f)
		{
			WanderDest = PickWanderPoint();
			WanderUntil = Now + FMath::FRandRange(4.f, 8.f);
		}
		MoveTowards(WanderDest, 100.f);
		break;
	}

	// Corre quando está longe do destino ou fugindo/carregando o queijo.
	if (URatMovementComponent* Move = Self->GetRatMovement())
	{
		const bool bUrgent = Goal == ERatBotGoal::GetCheese || Goal == ERatBotGoal::ChaseCarrier || Goal == ERatBotGoal::Flee || Goal == ERatBotGoal::HoldCheese;
		Move->SetSprinting(bUrgent && FVector::Dist(Self->GetActorLocation(), CurrentMoveDest) > 400.f);
	}
}

void ARatBotController::MoveTowards(const FVector& Dest, float Acceptance)
{
	const double Now = GetWorld()->GetTimeSeconds();
	const bool bFarFromLast = FVector::DistSquared(Dest, CurrentMoveDest) > FMath::Square(150.f);
	if (bFarFromLast || Now - LastMoveRequest > 1.5 || GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		CurrentMoveDest = Dest;
		LastMoveRequest = Now;
		MoveToLocation(Dest, Acceptance, true, true, true, true, nullptr, true);
	}
}

void ARatBotController::HandleCombat(ARatCharacter* Target, float DeltaSeconds)
{
	ARatCharacter* Self = GetRat();
	if (!Self)
	{
		return;
	}
	const ARatuchoGameState* GS = GetWorld()->GetGameState<ARatuchoGameState>();
	if (!Target || Target->IsDead() || !GS || GS->Phase != ERatMatchPhase::InProgress)
	{
		ClearFocus(EAIFocusPriority::Gameplay);
		Self->SetCombatFacing(false);
		return;
	}
	const FEnemyMemory* M = FindMemory(Target);
	if (!M || !M->bVisibleNow)
	{
		ClearFocus(EAIFocusPriority::Gameplay);
		Self->SetCombatFacing(false);
		return;
	}

	SetFocus(Target, EAIFocusPriority::Gameplay);
	Self->SetCombatFacing(true);

	const double Now = GetWorld()->GetTimeSeconds();
	URatWeaponComponent* Weapon = Self->GetWeapon();
	const FRatWeaponStats& Stats = Weapon->GetStats();
	const FVector Muzzle = Weapon->GetMuzzleLocation();
	const float Dist = FVector::Dist(Muzzle, Target->GetActorLocation());
	if (Dist > Stats.BotEngageRange || Now - M->FirstSeen < ReactionTime)
	{
		return;
	}

	// Mira com antecipação + compensação de gravidade + erro humano (varia a cada 0,4 s).
	if (Now >= NextAimAdjust)
	{
		NextAimAdjust = Now + 0.4;
		const float ErrDeg = Profile.AimErrorDegrees * (0.6f + Dist / 3000.f) * (Target->GetVelocity().Size2D() > 300.f ? 1.4f : 1.f);
		AimOffset = FMath::VRand() * FMath::Tan(FMath::DegreesToRadians(ErrDeg)) * Dist;
	}
	const float Time = Dist / FMath::Max(1.f, Stats.ProjectileSpeed);
	FVector AimPoint = Target->GetActorLocation() + Target->GetVelocity() * Time * 0.8f + AimOffset;
	AimPoint.Z += 0.5f * 980.f * Stats.ProjectileGravityScale * Time * Time;
	const FVector Dir = (AimPoint - Muzzle).GetSafeNormal();

	// Só atira se a linha até o alvo estiver livre (não desperdiça munição na parede).
	FCollisionQueryParams Q(SCENE_QUERY_STAT(BotFire), false, Self);
	Q.AddIgnoredActor(Target);
	if (GetWorld()->LineTraceTestByChannel(Muzzle, Target->GetActorLocation(), ECC_Visibility, Q))
	{
		return;
	}
	Weapon->FireFromServer(Dir);

	// Ocasionalmente pula durante o combate (comportamento de jogador).
	if (FMath::FRand() < 0.01f * Profile.Aggression)
	{
		Self->Jump();
	}
}

FVector ARatBotController::PickWanderPoint() const
{
	TArray<FVector> Candidates;
	for (const ARatSpawnPoint* P : TActorRange<ARatSpawnPoint>(GetWorld()))
	{
		Candidates.Add(P->GetActorLocation());
	}
	for (const ACheeseSpawnPoint* P : TActorRange<ACheeseSpawnPoint>(GetWorld()))
	{
		Candidates.Add(P->GetActorLocation());
	}
	if (Candidates.Num() == 0)
	{
		return GetRat()->GetActorLocation();
	}
	return Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
}

FVector ARatBotController::PickFleePoint(const FVector& Threat) const
{
	FVector Best = GetRat()->GetActorLocation();
	float BestScore = -MAX_flt;
	for (const ARatSpawnPoint* P : TActorRange<ARatSpawnPoint>(GetWorld()))
	{
		const FVector L = P->GetActorLocation();
		const float Score = FVector::Dist(L, Threat) - FVector::Dist(L, GetRat()->GetActorLocation()) * 0.5f + FMath::FRandRange(0.f, 300.f);
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = L;
		}
	}
	return Best;
}

FVector ARatBotController::PickCarrierHidePoint() const
{
	// Ponto longe dos inimigos lembrados (portador quer sobreviver até o fim).
	FVector Threat = GetRat()->GetActorLocation();
	if (Memory.Num() > 0)
	{
		FVector Sum = FVector::ZeroVector;
		for (const FEnemyMemory& M : Memory)
		{
			Sum += M.LastKnown;
		}
		Threat = Sum / Memory.Num();
		return PickFleePoint(Threat);
	}
	return PickWanderPoint();
}
