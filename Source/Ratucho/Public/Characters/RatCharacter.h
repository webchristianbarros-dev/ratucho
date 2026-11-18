#pragma once

#include "CoreMinimal.h"
#include "Game/RatuchoTypes.h"
#include "GameFramework/Character.h"
#include "RatCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class URatMovementComponent;
class URatWeaponComponent;
class ACheese;
struct FInputActionValue;

/**
 * Rato jogável (jogador humano ou bot) — mesmo personagem e mesmas regras para ambos.
 *
 * Autoridade: vida, dano, morte e invulnerabilidade são alterados só no servidor
 * e replicados. O cliente local envia input (movimento via CharacterMovement,
 * disparo/queijo via RPC Server*).
 */
UCLASS(Blueprintable)
class RATUCHO_API ARatCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ARatCharacter(const FObjectInitializer& ObjectInitializer);

	// --- Acesso ------------------------------------------------------------
	UFUNCTION(BlueprintPure, Category = "Ratucho") URatMovementComponent* GetRatMovement() const { return RatMovement; }
	UFUNCTION(BlueprintPure, Category = "Ratucho") URatWeaponComponent* GetWeapon() const { return Weapon; }
	UFUNCTION(BlueprintPure, Category = "Ratucho") float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category = "Ratucho") float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure, Category = "Ratucho") bool IsDead() const { return bIsDead; }
	UFUNCTION(BlueprintPure, Category = "Ratucho") bool IsInvulnerable() const;
	UFUNCTION(BlueprintPure, Category = "Ratucho") bool IsCarryingCheese() const { return bCarryingCheese; }
	USceneComponent* GetCheeseSocket() const { return CheeseSocket; }

	/** Nome legível para HUD/feed. */
	FString GetDisplayName() const;

	// --- Regras (servidor) ---------------------------------------------------
	/** Chamado pelo GameMode logo após o spawn. */
	void ServerInitForRound(ERatWeaponType WeaponType, float InvulnerabilitySeconds);

	/** Congela o rato no fim da rodada. */
	void ServerFreeze();

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void FellOutOfWorld(const UDamageType& DmgType) override;

	// --- Notificações (todas as máquinas) ------------------------------------
	void OnCheeseCarryChanged(bool bCarrying);
	void UpdateWeaponVisual(ERatWeaponType WeaponType);
	/** Local: o jogador apertou o gatilho e um disparo foi pedido (vira para a mira). */
	void NotifyLocalFired();
	/** Bots (servidor): encara a mira enquanto luta. */
	void SetCombatFacing(bool bEnable);

	// --- ACharacter ----------------------------------------------------------
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnJumped_Implementation() override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Parâmetros ------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Camera")
	float LookSensitivity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ratucho|Visual")
	float HardLandingSpeed = 900.f;

	// --- Componentes -----------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Camera") TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Camera") TObjectPtr<UCameraComponent> FollowCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Combat") TObjectPtr<URatWeaponComponent> Weapon;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Visual") TObjectPtr<USceneComponent> VisualRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Visual") TObjectPtr<USceneComponent> CheeseSocket;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Visual") TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Visual") TObjectPtr<UStaticMeshComponent> HeadMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Visual") TObjectPtr<UStaticMeshComponent> EarLeftMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Visual") TObjectPtr<UStaticMeshComponent> EarRightMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Visual") TObjectPtr<UStaticMeshComponent> NoseMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Visual") TObjectPtr<UStaticMeshComponent> TailMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Visual") TObjectPtr<UStaticMeshComponent> WeaponMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ratucho|Visual") TObjectPtr<UStaticMeshComponent> WeaponTipMesh;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool CanJumpInternal_Implementation() const override;

	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_SprintStarted(const FInputActionValue& Value);
	void Input_SprintCompleted(const FInputActionValue& Value);
	void Input_JumpStarted(const FInputActionValue& Value);
	void Input_JumpCompleted(const FInputActionValue& Value);
	void Input_FireStarted(const FInputActionValue& Value);
	void Input_FireCompleted(const FInputActionValue& Value);
	void Input_Interact(const FInputActionValue& Value);
	void Input_ThrowCheese(const FInputActionValue& Value);
	void Input_Reload(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable) void ServerInteract();
	UFUNCTION(Server, Reliable) void ServerThrowCheese();

	UFUNCTION() void OnRep_Health(float OldHealth);
	UFUNCTION() void OnRep_IsDead();

private:
	void Die(AController* Killer);
	void ApplyColors();
	ACheese* FindCheese() const;
	UStaticMeshComponent* CreatePart(const TCHAR* Name, UStaticMesh* PartMesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale);

	UPROPERTY(Transient) TObjectPtr<URatMovementComponent> RatMovement;

	UPROPERTY(ReplicatedUsing = OnRep_Health) float Health = 100.f;
	UPROPERTY(ReplicatedUsing = OnRep_IsDead) bool bIsDead = false;
	UPROPERTY(Replicated) double InvulnerableUntil = 0.0;

	/** Último atacante válido (para crédito de eliminação). Só no servidor. */
	TWeakObjectPtr<AController> LastDamager;

	bool bCarryingCheese = false;
	double CombatFacingUntil = 0.0;
	bool bCombatFacing = false;

	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> FurMIDs;
	FLinearColor FurColor = FLinearColor(0.5f, 0.45f, 0.42f);
	float DamageFlash = 0.f;
	FVector VisualScale = FVector::OneVector;
	float LastFallSpeed = 0.f;
	float DeathAnim = 0.f;
};
