#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RatSpawnPoints.generated.h"

class UCapsuleComponent;
class UArrowComponent;

/** Ponto de (re)spawn de ratos. Coloque com a base no chão; a cápsula mostra o espaço ocupado. */
UCLASS(Blueprintable)
class RATUCHO_API ARatSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ARatSpawnPoint();

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Ratucho") TObjectPtr<UCapsuleComponent> Preview;
	UPROPERTY(VisibleAnywhere, Category = "Ratucho") TObjectPtr<UArrowComponent> Arrow;
#endif
};

/** Local candidato para o surgimento inicial do queijo (sorteado no começo da rodada). */
UCLASS(Blueprintable)
class RATUCHO_API ACheeseSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ACheeseSpawnPoint();

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Ratucho") TObjectPtr<UCapsuleComponent> Preview;
#endif
};
