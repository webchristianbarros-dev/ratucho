#include "Characters/RatCharacter.h"

#include "AI/RatBotController.h"
#include "Camera/CameraComponent.h"
#include "Characters/RatMovementComponent.h"
#include "Combat/RatWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/RatuchoGameMode.h"
#include "Game/RatuchoGameState.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Gameplay/Cheese.h"
#include "InputActionValue.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Player/RatPlayerController.h"
#include "Player/RatPlayerState.h"
#include "UI/RatSfx.h"
#include "UObject/ConstructorHelpers.h"

namespace RatDims
{
	// Rato ~68 cm no mundo do jogo; cozinha em escala ~8x (ver docs/BALANCE.md).
	constexpr float CapsuleRadius = 30.f;
	constexpr float CapsuleHalfHeight = 34.f;
}

ARatCharacter::ARatCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<URatMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	GetCapsuleComponent()->InitCapsuleSize(RatDims::CapsuleRadius, RatDims::CapsuleHalfHeight);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	RatMovement = Cast<URatMovementComponent>(GetCharacterMovement());

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	CameraBoom->TargetArmLength = 320.f;
	CameraBoom->SocketOffset = FVector(0.f, 45.f, 55.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 14.f;
	CameraBoom->ProbeChannel = ECC_Camera;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 14.f;
	CameraBoom->CameraLagMaxDistance = 60.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(85.f);

	Weapon = CreateDefaultSubobject<URatWeaponComponent>(TEXT("Weapon"));

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(RootComponent);
	VisualRoot->SetRelativeLocation(FVector(0.f, 0.f, -RatDims::CapsuleHalfHeight));

	CheeseSocket = CreateDefaultSubobject<USceneComponent>(TEXT("CheeseSocket"));
	CheeseSocket->SetupAttachment(VisualRoot);
	CheeseSocket->SetRelativeLocation(FVector(-14.f, 0.f, 72.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	BodyMesh     = CreatePart(TEXT("Body"),  Sphere.Object,   FVector(-4.f, 0.f, 26.f),   FRotator::ZeroRotator,      FVector(0.62f, 0.46f, 0.46f));
	HeadMesh     = CreatePart(TEXT("Head"),  Sphere.Object,   FVector(30.f, 0.f, 38.f),   FRotator::ZeroRotator,      FVector(0.36f, 0.32f, 0.32f));
	NoseMesh     = CreatePart(TEXT("Nose"),  Cone.Object,     FVector(50.f, 0.f, 36.f),   FRotator(-90.f, 0.f, 0.f),  FVector(0.14f, 0.14f, 0.18f));
	EarLeftMesh  = CreatePart(TEXT("EarL"),  Cylinder.Object, FVector(26.f, -14.f, 56.f), FRotator(0.f, 0.f, -20.f),  FVector(0.20f, 0.20f, 0.03f));
	EarRightMesh = CreatePart(TEXT("EarR"),  Cylinder.Object, FVector(26.f, 14.f, 56.f),  FRotator(0.f, 0.f, 20.f),   FVector(0.20f, 0.20f, 0.03f));
	TailMesh     = CreatePart(TEXT("Tail"),  Cylinder.Object, FVector(-48.f, 0.f, 18.f),  FRotator(60.f, 0.f, 0.f),   FVector(0.04f, 0.04f, 0.50f));
	WeaponMesh   = CreatePart(TEXT("WeaponBody"), Cylinder.Object, FVector(30.f, 24.f, 30.f), FRotator(-90.f, 0.f, 0.f), FVector(0.1f, 0.1f, 0.4f));
	WeaponTipMesh = CreatePart(TEXT("WeaponTip"), Cylinder.Object, FVector(52.f, 24.f, 30.f), FRotator(-90.f, 0.f, 0.f), FVector(0.13f, 0.13f, 0.08f));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SolidMat(TEXT("/Game/Ratucho/Materials/Master/M_RatSolid.M_RatSolid"));
	if (SolidMat.Succeeded())
	{
		for (UStaticMeshComponent* Part : { BodyMesh.Get(), HeadMesh.Get(), NoseMesh.Get(), EarLeftMesh.Get(), EarRightMesh.Get(), TailMesh.Get(), WeaponMesh.Get(), WeaponTipMesh.Get() })
		{
			Part->SetMaterial(0, SolidMat.Object);
		}
	}

	GetMesh()->SetVisibility(false);
}

UStaticMeshComponent* ARatCharacter::CreatePart(const TCHAR* Name, UStaticMesh* PartMesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
{
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Part->SetupAttachment(VisualRoot);
	Part->SetStaticMesh(PartMesh);
	Part->SetRelativeLocation(Loc);
	Part->SetRelativeRotation(Rot);
	Part->SetRelativeScale3D(Scale);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetGenerateOverlapEvents(false);
	Part->SetCanEverAffectNavigation(false);
	Part->bReceivesDecals = false;
	return Part;
}

void ARatCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARatCharacter, Health);
	DOREPLIFETIME(ARatCharacter, bIsDead);
	DOREPLIFETIME(ARatCharacter, InvulnerableUntil);
}

void ARatCharacter::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	ApplyColors();
	UpdateWeaponVisual(Weapon ? Weapon->GetWeaponType() : ERatWeaponType::Slingshot);
}

void ARatCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Portador destruído (desconexão, troca de mapa): o queijo cai onde ele estava.
	if (HasAuthority() && bCarryingCheese)
	{
		if (ACheese* Cheese = FindCheese())
		{
			if (Cheese->GetCarrier() == this)
			{
				Cheese->DropAt(GetActorLocation(), true);
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}

ACheese* ARatCharacter::FindCheese() const
{
	const ARatuchoGameState* GS = GetWorld() ? GetWorld()->GetGameState<ARatuchoGameState>() : nullptr;
	return GS ? GS->Cheese : nullptr;
}

bool ARatCharacter::IsInvulnerable() const
{
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return GS && GS->GetServerWorldTimeSeconds() < InvulnerableUntil;
}

FString ARatCharacter::GetDisplayName() const
{
	const APlayerState* PS = GetPlayerState();
	return PS ? PS->GetPlayerName() : TEXT("Rato");
}

// ============================================================================
// Regras (servidor)
// ============================================================================

void ARatCharacter::ServerInitForRound(ERatWeaponType WeaponType, float InvulnerabilitySeconds)
{
	if (!HasAuthority())
	{
		return;
	}
	Health = MaxHealth;
	bIsDead = false;
	LastDamager.Reset();
	const AGameStateBase* GS = GetWorld()->GetGameState();
	InvulnerableUntil = (GS ? GS->GetServerWorldTimeSeconds() : 0.0) + InvulnerabilitySeconds;
	if (Weapon)
	{
		Weapon->ServerSetWeapon(WeaponType);
	}
}

void ARatCharacter::ServerFreeze()
{
	if (!HasAuthority())
	{
		return;
	}
	if (Weapon)
	{
		Weapon->ServerResetState();
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
}

float ARatCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || bIsDead || DamageAmount <= 0.f || IsInvulnerable())
	{
		return 0.f;
	}
	if (const ARatuchoGameState* GS = GetWorld()->GetGameState<ARatuchoGameState>())
	{
		if (GS->Phase != ERatMatchPhase::InProgress)
		{
			return 0.f;
		}
	}
	// Sem dano próprio na primeira versão.
	if (EventInstigator && EventInstigator == GetController())
	{
		return 0.f;
	}

	const float Applied = FMath::Min(Health, DamageAmount);
	const float Old = Health;
	Health -= Applied;
	if (EventInstigator)
	{
		LastDamager = EventInstigator;
		if (ARatBotController* Bot = Cast<ARatBotController>(GetController()))
		{
			Bot->NotifyDamagedBy(EventInstigator);
		}
	}
	OnRep_Health(Old); // no servidor RepNotify não dispara sozinho

	if (Health <= 0.f)
	{
		Die(EventInstigator);
	}
	return Applied;
}

void ARatCharacter::FellOutOfWorld(const UDamageType& DmgType)
{
	if (HasAuthority() && !bIsDead)
	{
		Health = 0.f;
		Die(nullptr); // queda: não conta como eliminação de outro participante
	}
}

void ARatCharacter::Die(AController* Killer)
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}
	bIsDead = true;

	if (bCarryingCheese)
	{
		if (ACheese* Cheese = FindCheese())
		{
			const float HH = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			Cheese->DropAt(GetActorLocation() - FVector(0.f, 0.f, HH * 0.5f), false);
		}
	}

	if (Weapon)
	{
		Weapon->ServerResetState();
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	OnRep_IsDead();

	if (ARatuchoGameMode* GM = GetWorld()->GetAuthGameMode<ARatuchoGameMode>())
	{
		GM->OnRatKilled(this, Killer);
	}
}

void ARatCharacter::OnRep_Health(float OldHealth)
{
	if (Health < OldHealth)
	{
		DamageFlash = 1.f;
		VisualScale = FVector(1.15f, 1.15f, 0.85f);
	}
}

void ARatCharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		// Corpo não bloqueia tiros nem outros ratos.
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		DeathAnim = 0.f;
		const FVector Loc = GetActorLocation();
		RatSfx::PlayCue(this, ERatSfx::Death, &Loc);
		if (WeaponMesh) { WeaponMesh->SetVisibility(false); }
		if (WeaponTipMesh) { WeaponTipMesh->SetVisibility(false); }
	}
}

// ============================================================================
// Ações do jogador
// ============================================================================

void ARatCharacter::ServerInteract_Implementation()
{
	if (bIsDead)
	{
		return;
	}
	if (ACheese* Cheese = FindCheese())
	{
		Cheese->TryPickup(this, true); // servidor valida alcance, posse única e fase
	}
}

void ARatCharacter::ServerThrowCheese_Implementation()
{
	if (bIsDead)
	{
		return;
	}
	if (ACheese* Cheese = FindCheese())
	{
		Cheese->Throw(this); // só funciona se este rato for o portador
	}
}

void ARatCharacter::OnCheeseCarryChanged(bool bCarrying)
{
	bCarryingCheese = bCarrying;
}

void ARatCharacter::NotifyLocalFired()
{
	CombatFacingUntil = GetWorld()->GetTimeSeconds() + 0.9;
	SetCombatFacing(true);
}

void ARatCharacter::SetCombatFacing(bool bEnable)
{
	if (bCombatFacing == bEnable || !RatMovement)
	{
		return;
	}
	bCombatFacing = bEnable;
	RatMovement->bOrientRotationToMovement = !bEnable;
	RatMovement->bUseControllerDesiredRotation = bEnable;
}

void ARatCharacter::UpdateWeaponVisual(ERatWeaponType WeaponType)
{
	if (!WeaponMesh || !WeaponTipMesh)
	{
		return;
	}
	if (WeaponType == ERatWeaponType::CapBlaster)
	{
		// Tubo verde comprido (lançador de tampinhas).
		WeaponMesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.5f));
		WeaponMesh->SetRelativeLocation(FVector(34.f, 24.f, 30.f));
		WeaponTipMesh->SetRelativeScale3D(FVector(0.17f, 0.17f, 0.1f));
		WeaponTipMesh->SetRelativeLocation(FVector(60.f, 24.f, 30.f));
	}
	else
	{
		// Estilingue: cabo curto de madeira + forquilha.
		WeaponMesh->SetRelativeScale3D(FVector(0.07f, 0.07f, 0.3f));
		WeaponMesh->SetRelativeLocation(FVector(30.f, 24.f, 26.f));
		WeaponTipMesh->SetRelativeScale3D(FVector(0.2f, 0.05f, 0.14f));
		WeaponTipMesh->SetRelativeLocation(FVector(46.f, 24.f, 34.f));
	}
	ApplyColors();
}

void ARatCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	ApplyColors();
}

void ARatCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	ApplyColors();
}

void ARatCharacter::ApplyColors()
{
	if (const ARatPlayerState* PS = GetPlayerState<ARatPlayerState>())
	{
		FurColor = PS->RatColor;
	}
	const bool bCaps = Weapon && Weapon->GetWeaponType() == ERatWeaponType::CapBlaster;
	const FLinearColor Pink(0.95f, 0.55f, 0.6f);
	const FLinearColor WeaponBody = bCaps ? FLinearColor(0.35f, 0.9f, 0.2f) : FLinearColor(0.45f, 0.28f, 0.12f);
	const FLinearColor WeaponTip = bCaps ? FLinearColor(0.15f, 0.3f, 0.15f) : FLinearColor(0.35f, 0.2f, 0.1f);

	FurMIDs.Reset();
	auto Paint = [this](UStaticMeshComponent* Part, const FLinearColor& C, bool bFur)
	{
		if (!Part)
		{
			return;
		}
		if (UMaterialInstanceDynamic* MID = Part->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), C);
			if (bFur)
			{
				FurMIDs.Add(MID);
			}
		}
	};
	Paint(BodyMesh, FurColor, true);
	Paint(HeadMesh, FurColor, true);
	Paint(EarLeftMesh, Pink, false);
	Paint(EarRightMesh, Pink, false);
	Paint(NoseMesh, Pink, false);
	Paint(TailMesh, Pink, false);
	Paint(WeaponMesh, WeaponBody, false);
	Paint(WeaponTipMesh, WeaponTip, false);
}

// ============================================================================
// Input
// ============================================================================

void ARatCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	ARatPlayerController* PC = Cast<ARatPlayerController>(GetController());
	if (!EIC || !PC)
	{
		return;
	}
	const FRatInputActions& A = PC->GetInputActions();
	EIC->BindAction(A.Move, ETriggerEvent::Triggered, this, &ARatCharacter::Input_Move);
	EIC->BindAction(A.Look, ETriggerEvent::Triggered, this, &ARatCharacter::Input_Look);
	EIC->BindAction(A.Jump, ETriggerEvent::Started, this, &ARatCharacter::Input_JumpStarted);
	EIC->BindAction(A.Jump, ETriggerEvent::Completed, this, &ARatCharacter::Input_JumpCompleted);
	EIC->BindAction(A.Sprint, ETriggerEvent::Started, this, &ARatCharacter::Input_SprintStarted);
	EIC->BindAction(A.Sprint, ETriggerEvent::Completed, this, &ARatCharacter::Input_SprintCompleted);
	EIC->BindAction(A.Fire, ETriggerEvent::Started, this, &ARatCharacter::Input_FireStarted);
	EIC->BindAction(A.Fire, ETriggerEvent::Completed, this, &ARatCharacter::Input_FireCompleted);
	EIC->BindAction(A.Interact, ETriggerEvent::Started, this, &ARatCharacter::Input_Interact);
	EIC->BindAction(A.ThrowCheese, ETriggerEvent::Started, this, &ARatCharacter::Input_ThrowCheese);
	EIC->BindAction(A.Reload, ETriggerEvent::Started, this, &ARatCharacter::Input_Reload);
}

void ARatCharacter::Input_Move(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>();
	if (!Controller || bIsDead)
	{
		return;
	}
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), V.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), V.X);
}

void ARatCharacter::Input_Look(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>() * LookSensitivity;
	AddControllerYawInput(V.X);
	AddControllerPitchInput(V.Y);
}

void ARatCharacter::Input_SprintStarted(const FInputActionValue&)   { if (RatMovement) { RatMovement->SetSprinting(true); } }
void ARatCharacter::Input_SprintCompleted(const FInputActionValue&) { if (RatMovement) { RatMovement->SetSprinting(false); } }
void ARatCharacter::Input_JumpStarted(const FInputActionValue&)     { if (!bIsDead) { Jump(); } }
void ARatCharacter::Input_JumpCompleted(const FInputActionValue&)   { StopJumping(); }
void ARatCharacter::Input_FireStarted(const FInputActionValue&)     { if (Weapon) { Weapon->SetTriggerHeld(true); } }
void ARatCharacter::Input_FireCompleted(const FInputActionValue&)   { if (Weapon) { Weapon->SetTriggerHeld(false); } }
void ARatCharacter::Input_Interact(const FInputActionValue&)        { ServerInteract(); }
void ARatCharacter::Input_ThrowCheese(const FInputActionValue&)     { if (bCarryingCheese) { ServerThrowCheese(); } }
void ARatCharacter::Input_Reload(const FInputActionValue&)          { if (Weapon) { Weapon->RequestReload(); } }

bool ARatCharacter::CanJumpInternal_Implementation() const
{
	if (bIsDead)
	{
		return false;
	}
	if (RatMovement && RatMovement->IsClimbing())
	{
		return true;
	}
	return Super::CanJumpInternal_Implementation();
}

// ============================================================================
// Feedback visual
// ============================================================================

void ARatCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();
	VisualScale = FVector(0.85f, 0.85f, 1.25f);
}

void ARatCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	const float Impact = FMath::Clamp(LastFallSpeed / HardLandingSpeed, 0.f, 1.f);
	VisualScale = FVector(1.05f + 0.3f * Impact, 1.05f + 0.3f * Impact, 0.95f - 0.4f * Impact);
}

void ARatCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	if (GetLocalRole() == ROLE_SimulatedProxy && RatMovement)
	{
		if (PrevMovementMode == MOVE_Falling && RatMovement->IsMovingOnGround())
		{
			VisualScale = FVector(1.15f, 1.15f, 0.8f);
		}
		else if (RatMovement->IsFalling() && GetVelocity().Z > 100.f)
		{
			VisualScale = FVector(0.85f, 0.85f, 1.25f);
		}
	}
}

void ARatCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (RatMovement && RatMovement->IsFalling())
	{
		LastFallSpeed = FMath::Max(0.f, -GetVelocity().Z);
	}

	// Jogador local: volta a orientar pelo movimento depois de parar de atirar.
	if (IsLocallyControlled() && !IsBotControlled() && bCombatFacing && GetWorld()->GetTimeSeconds() > CombatFacingUntil)
	{
		SetCombatFacing(false);
	}

	VisualScale = FMath::VInterpTo(VisualScale, FVector::OneVector, DeltaSeconds, 10.f);

	if (VisualRoot)
	{
		FRotator Target(0.f, 0.f, 0.f);
		if (bIsDead)
		{
			DeathAnim = FMath::Min(1.f, DeathAnim + DeltaSeconds * 4.f);
			Target = FRotator(0.f, 0.f, 90.f * DeathAnim); // tomba de lado
		}
		else if (RatMovement && RatMovement->IsClimbing())
		{
			Target = FRotator(-70.f, 0.f, 0.f);
		}
		VisualRoot->SetRelativeScale3D(VisualScale);
		VisualRoot->SetRelativeRotation(FMath::RInterpTo(VisualRoot->GetRelativeRotation(), Target, DeltaSeconds, 12.f));

		// Invulnerabilidade pós-respawn: pisca discretamente.
		const bool bBlink = !bIsDead && IsInvulnerable() && FMath::Fmod(GetWorld()->GetTimeSeconds(), 0.25) < 0.08;
		VisualRoot->SetVisibility(!bBlink, true);
		if (bIsDead)
		{
			if (WeaponMesh) { WeaponMesh->SetVisibility(false); }
			if (WeaponTipMesh) { WeaponTipMesh->SetVisibility(false); }
		}
	}

	// Flash vermelho ao levar dano.
	if (DamageFlash > 0.f)
	{
		DamageFlash = FMath::Max(0.f, DamageFlash - DeltaSeconds * 5.f);
		const FLinearColor C = FMath::Lerp(FurColor, FLinearColor(1.f, 0.1f, 0.05f), DamageFlash);
		for (UMaterialInstanceDynamic* MID : FurMIDs)
		{
			if (MID)
			{
				MID->SetVectorParameterValue(TEXT("Color"), C);
			}
		}
	}
}
