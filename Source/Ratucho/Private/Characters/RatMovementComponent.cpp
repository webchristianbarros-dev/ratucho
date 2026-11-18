#include "Characters/RatMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "World/ClimbVolume.h"

// ============================================================================
// Saved move / prediction data
// ============================================================================

class FSavedMove_Rat : public FSavedMove_Character
{
public:
	typedef FSavedMove_Character Super;

	uint8 bSavedWantsToSprint : 1 = 0;

	virtual void Clear() override
	{
		Super::Clear();
		bSavedWantsToSprint = 0;
	}

	virtual uint8 GetCompressedFlags() const override
	{
		uint8 Result = Super::GetCompressedFlags();
		if (bSavedWantsToSprint)
		{
			Result |= FLAG_Custom_0;
		}
		return Result;
	}

	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override
	{
		const FSavedMove_Rat* Other = static_cast<const FSavedMove_Rat*>(NewMove.Get());
		if (bSavedWantsToSprint != Other->bSavedWantsToSprint)
		{
			return false;
		}
		return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
	}

	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override
	{
		Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
		if (const URatMovementComponent* Move = Cast<URatMovementComponent>(C->GetCharacterMovement()))
		{
			bSavedWantsToSprint = Move->bWantsToSprint;
		}
	}

	virtual void PrepMoveFor(ACharacter* C) override
	{
		Super::PrepMoveFor(C);
		if (URatMovementComponent* Move = Cast<URatMovementComponent>(C->GetCharacterMovement()))
		{
			Move->bWantsToSprint = bSavedWantsToSprint;
		}
	}
};

class FNetworkPredictionData_Client_Rat : public FNetworkPredictionData_Client_Character
{
public:
	explicit FNetworkPredictionData_Client_Rat(const UCharacterMovementComponent& ClientMovement)
		: FNetworkPredictionData_Client_Character(ClientMovement)
	{
	}

	virtual FSavedMovePtr AllocateNewMove() override
	{
		return FSavedMovePtr(new FSavedMove_Rat());
	}
};

// ============================================================================
// URatMovementComponent
// ============================================================================

URatMovementComponent::URatMovementComponent()
{
	bWantsToSprint = false;

	// Valores iniciais de protótipo (ver docs/BALANCE.md). Tudo editável no Blueprint.
	MaxWalkSpeed = 440.f;
	MaxAcceleration = 2600.f;
	BrakingDecelerationWalking = 2600.f;
	GroundFriction = 8.f;
	BrakingFrictionFactor = 1.f;
	JumpZVelocity = 640.f;
	AirControl = 0.45f;
	GravityScale = 1.6f;
	MaxStepHeight = 40.f;
	SetWalkableFloorAngle(50.f);
	RotationRate = FRotator(0.f, 720.f, 0.f);
	bOrientRotationToMovement = true;
	bUseControllerDesiredRotation = false;
	NavAgentProps.bCanJump = true;
	NavAgentProps.bCanCrouch = false;
	SetIsReplicatedByDefault(true);
}

bool URatMovementComponent::IsSprinting() const
{
	if (!bWantsToSprint || !IsMovingOnGround() || !CharacterOwner)
	{
		return false;
	}
	const FVector Dir = GetInputDirection2D();
	return !Dir.IsNearlyZero() && FVector::DotProduct(Dir, CharacterOwner->GetActorForwardVector()) >= SprintMinForwardDot;
}

bool URatMovementComponent::IsClimbing() const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == static_cast<uint8>(ERatCustomMovement::Climb);
}

float URatMovementComponent::GetMaxSpeed() const
{
	if (IsClimbing())
	{
		return ClimbSpeed;
	}
	if (IsSprinting())
	{
		return SprintSpeed;
	}
	return Super::GetMaxSpeed();
}

float URatMovementComponent::GetMaxAcceleration() const
{
	return IsSprinting() ? SprintAcceleration : Super::GetMaxAcceleration();
}

bool URatMovementComponent::CanAttemptJump() const
{
	return Super::CanAttemptJump() || (IsJumpAllowed() && IsClimbing());
}

bool URatMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
{
	if (IsClimbing() && CharacterOwner)
	{
		// Pulo de parede: afasta da superfície e sobe.
		Velocity = LastClimbNormal * WallJumpOutSpeed + FVector::UpVector * WallJumpUpSpeed;
		SetMovementMode(MOVE_Falling);
		return true;
	}
	return Super::DoJump(bReplayingMoves, DeltaTime);
}

void URatMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

FNetworkPredictionData_Client* URatMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr);
	if (ClientPredictionData == nullptr)
	{
		URatMovementComponent* MutableThis = const_cast<URatMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Rat(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
	}
	return ClientPredictionData;
}

FVector URatMovementComponent::GetInputDirection2D() const
{
	const float MaxAccel = FMath::Max(Super::GetMaxAcceleration(), 1.f);
	FVector Dir = Acceleration / MaxAccel;
	Dir.Z = 0.f;
	return Dir.GetClampedToMaxSize(1.f);
}

AClimbVolume* URatMovementComponent::FindClimbVolume() const
{
	if (!CharacterOwner || !GetWorld())
	{
		return nullptr;
	}
	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	const FVector Loc = UpdatedComponent->GetComponentLocation();
	const float R = Capsule->GetScaledCapsuleRadius();
	const float HH = Capsule->GetScaledCapsuleHalfHeight();

	for (AClimbVolume* Volume : TActorRange<AClimbVolume>(GetWorld()))
	{
		if (Volume && Volume->ContainsCapsule(Loc, R, HH))
		{
			return Volume;
		}
	}
	return nullptr;
}

bool URatMovementComponent::ShouldEnterClimb(const AClimbVolume* Volume) const
{
	if (!Volume || !(IsMovingOnGround() || IsFalling()))
	{
		return false;
	}
	if (IsFalling() && Velocity.Z > ClimbEnterMaxUpSpeed)
	{
		return false;
	}
	const FVector Dir = GetInputDirection2D();
	if (Dir.SizeSquared() < 0.25f)
	{
		return false;
	}
	// Input empurrando contra a parede (oposto à normal).
	return FVector::DotProduct(Dir.GetSafeNormal(), -Volume->GetSurfaceNormal()) >= ClimbEnterInputDot;
}

void URatMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	if (!IsClimbing() && CharacterOwner && CharacterOwner->GetLocalRole() > ROLE_SimulatedProxy)
	{
		if (AClimbVolume* Volume = FindClimbVolume())
		{
			if (ShouldEnterClimb(Volume))
			{
				LastClimbNormal = Volume->GetSurfaceNormal();
				Velocity = FVector::ZeroVector;
				SetMovementMode(MOVE_Custom, static_cast<uint8>(ERatCustomMovement::Climb));
			}
		}
	}
}

void URatMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	if (CustomMovementMode == static_cast<uint8>(ERatCustomMovement::Climb))
	{
		PhysClimb(DeltaTime, Iterations);
		return;
	}
	Super::PhysCustom(DeltaTime, Iterations);
}

void URatMovementComponent::PhysClimb(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	const FVector Dir = GetInputDirection2D();
	AClimbVolume* Volume = FindClimbVolume();

	if (!Volume)
	{
		// Saiu do volume: se estava subindo, "sobe a borda" para cima da plataforma.
		const float UpInput = FVector::DotProduct(Dir, -LastClimbNormal);
		if (UpInput > 0.1f)
		{
			Velocity = -LastClimbNormal * MantleForwardSpeed + FVector::UpVector * MantleUpSpeed;
		}
		else
		{
			Velocity = LastClimbNormal * 100.f;
		}
		SetMovementMode(MOVE_Falling);
		StartNewPhysics(DeltaTime, Iterations);
		return;
	}

	const FVector N = Volume->GetSurfaceNormal();
	LastClimbNormal = N;
	const FVector Facing = -N;
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Facing).GetSafeNormal();

	const float UpInput = FVector::DotProduct(Dir, Facing);
	const float SideInput = FVector::DotProduct(Dir, Right);

	// Descendo e chegou ao chão: volta a andar.
	if (UpInput < -0.1f)
	{
		FFindFloorResult Floor;
		FindFloor(UpdatedComponent->GetComponentLocation(), Floor, false);
		if (Floor.IsWalkableFloor() && Floor.FloorDist <= MAX_FLOOR_DIST + 2.f)
		{
			SetMovementMode(MOVE_Walking);
			StartNewPhysics(DeltaTime, Iterations);
			return;
		}
	}

	const float Speed = ClimbSpeed * Volume->ClimbSpeedMultiplier;
	Velocity = FVector::UpVector * (UpInput * Speed) + Right * (SideInput * Speed * ClimbLateralFactor);

	// Pequena força contra a parede para não descolar.
	const FVector Stick = Facing * 60.f * DeltaTime;
	const FVector Delta = Velocity * DeltaTime + Stick;
	const FQuat TargetRot = Facing.ToOrientationQuat();

	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Delta, TargetRot, true, Hit);
	if (Hit.IsValidBlockingHit())
	{
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
	}
}

void URatMovementComponent::PhysicsRotation(float DeltaTime)
{
	if (IsClimbing())
	{
		return; // rotação definida em PhysClimb (de frente para a parede)
	}
	Super::PhysicsRotation(DeltaTime);
}
