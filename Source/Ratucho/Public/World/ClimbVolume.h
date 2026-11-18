#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ClimbVolume.generated.h"

class UBoxComponent;
class UArrowComponent;
class UStaticMeshComponent;

/**
 * Superfície de escalada explícita (o rato NÃO escala malhas arbitrárias).
 *
 * Posicione a caixa encostada na face escalável; a seta (eixo X do ator) deve
 * apontar PARA FORA da superfície. O topo da caixa deve coincidir com o topo
 * da plataforma, para que o rato "suba a borda" ao sair por cima.
 *
 * A detecção é puramente geométrica (posição da cápsula x caixa), então
 * cliente e servidor chegam ao mesmo resultado durante a predição de movimento.
 */
UCLASS(Blueprintable)
class RATUCHO_API AClimbVolume : public AActor
{
	GENERATED_BODY()

public:
	AClimbVolume();

	/** Normal horizontal da superfície escalável (para fora da parede). */
	FVector GetSurfaceNormal() const;

	/** True se uma cápsula com o centro em Location e as dimensões dadas toca o volume. */
	bool ContainsCapsule(const FVector& Location, float Radius, float HalfHeight) const;

	/** Altura (Z mundo) do topo do volume. */
	float GetTopZ() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Climb")
	TObjectPtr<UBoxComponent> Box;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Ratucho|Climb")
	TObjectPtr<UArrowComponent> Arrow;
#endif

	/** Faixa visual que indica ao jogador onde é possível escalar. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Climb")
	TObjectPtr<UStaticMeshComponent> Marker;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ratucho|Climb")
	bool bShowMarker = true;

	/** Multiplicador de velocidade de escalada nesta superfície. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ratucho|Climb", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float ClimbSpeedMultiplier = 1.f;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
