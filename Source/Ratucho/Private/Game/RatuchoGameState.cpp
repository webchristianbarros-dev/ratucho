#include "Game/RatuchoGameState.h"

#include "Characters/RatCharacter.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/Cheese.h"
#include "Net/UnrealNetwork.h"
#include "Player/RatPlayerState.h"
#include "RatuchoLog.h"
#include "UI/RatSfx.h"

void ARatuchoGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARatuchoGameState, Phase);
	DOREPLIFETIME(ARatuchoGameState, PhaseEndServerTime);
	DOREPLIFETIME(ARatuchoGameState, Cheese);
	DOREPLIFETIME(ARatuchoGameState, Winner);
	DOREPLIFETIME(ARatuchoGameState, RoundNumber);
	DOREPLIFETIME(ARatuchoGameState, RoundDuration);
	DOREPLIFETIME(ARatuchoGameState, RespawnDelay);
}

float ARatuchoGameState::GetPhaseTimeRemaining() const
{
	return static_cast<float>(FMath::Max(0.0, PhaseEndServerTime - GetServerWorldTimeSeconds()));
}

ARatPlayerState* ARatuchoGameState::GetCheeseCarrierState() const
{
	if (Cheese && Cheese->GetCarrier())
	{
		return Cheese->GetCarrier()->GetPlayerState<ARatPlayerState>();
	}
	return nullptr;
}

bool ARatuchoGameState::IsLocal(const APlayerState* PS) const
{
	if (!PS)
	{
		return false;
	}
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	return PC && PC->PlayerState == PS;
}

FString ARatuchoGameState::DescribeForLocal(const APlayerState* PS, bool bCapitalize) const
{
	if (!PS)
	{
		return bCapitalize ? TEXT("Alguém") : TEXT("alguém");
	}
	if (IsLocal(PS))
	{
		return bCapitalize ? TEXT("Você") : TEXT("você");
	}
	return PS->GetPlayerName();
}

void ARatuchoGameState::PushFeed(const FString& Text, const FLinearColor& Color)
{
	FRatFeedEntry E;
	E.Text = Text;
	E.Color = Color;
	E.Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	Feed.Add(E);
	if (Feed.Num() > 12)
	{
		Feed.RemoveAt(0);
	}
	UE_LOG(LogRatucho, Display, TEXT("[Feed][%s] %s"), HasAuthority() ? TEXT("Server") : TEXT("Client"), *Text);
}

void ARatuchoGameState::MulticastKillFeed_Implementation(APlayerState* Killer, APlayerState* Victim)
{
	FString Text;
	FLinearColor Color = FLinearColor::White;
	if (IsLocal(Victim))
	{
		LocalKilledBy = (Killer && Killer != Victim) ? Killer->GetPlayerName() : FString();
		LocalDeathTime = GetWorld()->GetTimeSeconds();
	}
	if (Killer && Killer != Victim)
	{
		Text = FString::Printf(TEXT("%s eliminou %s!"), *DescribeForLocal(Killer, true), *DescribeForLocal(Victim, false));
		if (IsLocal(Killer))
		{
			Color = FLinearColor(0.4f, 1.f, 0.4f);
		}
		else if (IsLocal(Victim))
		{
			Color = FLinearColor(1.f, 0.4f, 0.35f);
		}
	}
	else
	{
		Text = FString::Printf(TEXT("%s caiu e foi eliminado!"), *DescribeForLocal(Victim, true));
		Color = FLinearColor(0.8f, 0.8f, 0.8f);
	}
	PushFeed(Text, Color);
}

void ARatuchoGameState::MulticastCheeseFeed_Implementation(ERatCheeseEvent Event, APlayerState* Who)
{
	const FLinearColor Yellow(1.f, 0.85f, 0.2f);
	switch (Event)
	{
	case ERatCheeseEvent::Spawned:
		PushFeed(TEXT("O queijo apareceu na cozinha!"), Yellow);
		break;
	case ERatCheeseEvent::PickedUp:
		PushFeed(FString::Printf(TEXT("%s pegou o queijo!"), *DescribeForLocal(Who, true)), Yellow);
		RatSfx::PlayCue(this, IsLocal(Who) ? ERatSfx::CheeseMine : ERatSfx::CheeseOther);
		break;
	case ERatCheeseEvent::Thrown:
		PushFeed(FString::Printf(TEXT("%s jogou o queijo! O queijo está livre!"), *DescribeForLocal(Who, true)), Yellow);
		break;
	case ERatCheeseEvent::DroppedOnDeath:
		PushFeed(FString::Printf(TEXT("%s derrubou o queijo! O queijo está livre!"), *DescribeForLocal(Who, true)), Yellow);
		break;
	case ERatCheeseEvent::DroppedOnLeave:
		PushFeed(TEXT("O portador saiu. O queijo está livre!"), Yellow);
		break;
	}
}

void ARatuchoGameState::MulticastAnnouncement_Implementation(const FString& Text)
{
	PushFeed(Text, FLinearColor(0.7f, 0.85f, 1.f));
}
