#include "Player/RatPlayerState.h"

#include "Net/UnrealNetwork.h"

void ARatPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARatPlayerState, Kills);
	DOREPLIFETIME(ARatPlayerState, Deaths);
	DOREPLIFETIME(ARatPlayerState, RatColor);
	DOREPLIFETIME(ARatPlayerState, Weapon);
	DOREPLIFETIME(ARatPlayerState, Trait);
}
