#include "Combat/RatWeaponComponent.h"

#include "Camera/CameraComponent.h"
#include "Characters/RatCharacter.h"
#include "Combat/RatProjectile.h"
#include "Engine/World.h"
#include "Game/RatuchoGameMode.h"
#include "Game/RatuchoGameState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

URatWeaponComponent::URatWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	SlingshotStats = RatuchoDefaults::MakeWeaponStats(ERatWeaponType::Slingshot);
	CapBlasterStats = RatuchoDefaults::MakeWeaponStats(ERatWeaponType::CapBlaster);
	ProjectileClass = ARatProjectile::StaticClass();
}

void URatWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URatWeaponComponent, WeaponType);
	DOREPLIFETIME(URatWeaponComponent, Ammo);
	DOREPLIFETIME(URatWeaponComponent, bReloading);
	DOREPLIFETIME(URatWeaponComponent, ReloadStartServerTime);
}

const FRatWeaponStats& URatWeaponComponent::GetStats() const
{
	return WeaponType == ERatWeaponType::CapBlaster ? CapBlasterStats : SlingshotStats;
}

ARatCharacter* URatWeaponComponent::GetRat() const
{
	return Cast<ARatCharacter>(GetOwner());
}

float URatWeaponComponent::GetReloadProgress() const
{
	if (!bReloading)
	{
		return 1.f;
	}
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const double Now = GS ? GS->GetServerWorldTimeSeconds() : 0.0;
	return FMath::Clamp(static_cast<float>((Now - ReloadStartServerTime) / FMath::Max(0.01f, GetStats().ReloadTime)), 0.f, 1.f);
}

void URatWeaponComponent::ServerSetWeapon(ERatWeaponType NewType)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	WeaponType = NewType;
	ServerResetState();
	Ammo = GetStats().MagazineSize;
	OnRep_WeaponType();
}

void URatWeaponComponent::OnRep_WeaponType()
{
	if (ARatCharacter* Rat = GetRat())
	{
		Rat->UpdateWeaponVisual(WeaponType);
	}
}

void URatWeaponComponent::ServerResetState()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BurstTimer);
		World->GetTimerManager().ClearTimer(ReloadTimer);
	}
	PendingBurstShots = 0;
	bReloading = false;
	bTriggerHeld = false;
}

void URatWeaponComponent::SetTriggerHeld(bool bHeld)
{
	bTriggerHeld = bHeld;
}

void URatWeaponComponent::RequestReload()
{
	if (Ammo < GetStats().MagazineSize && !bReloading)
	{
		ServerReload();
	}
}

void URatWeaponComponent::ServerReload_Implementation()
{
	if (!bReloading && Ammo < GetStats().MagazineSize)
	{
		StartReload();
	}
}

FVector URatWeaponComponent::GetMuzzleLocation() const
{
	const ARatCharacter* Rat = GetRat();
	if (!Rat)
	{
		return FVector::ZeroVector;
	}
	// Saída na frente/direita do rato, à altura do peito, alinhada à mira (yaw de controle).
	const FRotator Yaw(0.f, Rat->GetBaseAimRotation().Yaw, 0.f);
	return Rat->GetActorLocation() + Yaw.RotateVector(FVector(45.f, 22.f, 18.f));
}

FVector URatWeaponComponent::ComputeLocalAimDirection() const
{
	const ARatCharacter* Rat = GetRat();
	if (!Rat || !Rat->FollowCamera)
	{
		return FVector::ForwardVector;
	}
	// Mira onde a câmera aponta: traça do centro da tela e mira o ponto atingido.
	const FVector CamLoc = Rat->FollowCamera->GetComponentLocation();
	const FVector CamDir = Rat->FollowCamera->GetForwardVector();
	const FVector TraceStart = CamLoc + CamDir * (Rat->CameraBoom ? 300.f : 0.f);
	const FVector TraceEnd = CamLoc + CamDir * GetStats().MaxRange;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RatAim), false, Rat);
	FVector Target = TraceEnd;
	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		Target = Hit.ImpactPoint;
	}
	FVector Dir = (Target - GetMuzzleLocation()).GetSafeNormal();
	// Compensa a gravidade do projétil levemente para alvos distantes.
	const float Dist = FVector::Dist(Target, GetMuzzleLocation());
	const float Time = Dist / FMath::Max(1.f, GetStats().ProjectileSpeed);
	const float Drop = 0.5f * 980.f * GetStats().ProjectileGravityScale * Time * Time;
	Dir = ((Target + FVector(0.f, 0.f, Drop)) - GetMuzzleLocation()).GetSafeNormal();
	return Dir;
}

bool URatWeaponComponent::CanFireNow(double Now) const
{
	const ARatCharacter* Rat = GetRat();
	if (!Rat || Rat->IsDead() || bReloading || Ammo <= 0 || PendingBurstShots > 0)
	{
		return false;
	}
	if (const ARatuchoGameState* GS = GetWorld()->GetGameState<ARatuchoGameState>())
	{
		if (GS->Phase != ERatMatchPhase::InProgress)
		{
			return false;
		}
	}
	// Tolerância de 15% para jitter de rede.
	return Now - LastServerFireTime >= GetStats().FireInterval * 0.85f;
}

void URatWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ARatCharacter* Rat = GetRat();
	if (!Rat || !Rat->IsLocallyControlled() || Rat->IsBotControlled())
	{
		return;
	}
	if (!bTriggerHeld || Rat->IsDead())
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastLocalFireTime < GetStats().FireInterval)
	{
		return;
	}
	if (Ammo <= 0)
	{
		if (!bReloading)
		{
			RequestReload();
		}
		return;
	}
	if (bReloading)
	{
		return;
	}
	LastLocalFireTime = Now;
	Rat->NotifyLocalFired();
	ServerFire(ComputeLocalAimDirection());
}

void URatWeaponComponent::ServerFire_Implementation(FVector_NetQuantizeNormal AimDirection)
{
	ARatCharacter* Rat = GetRat();
	if (!Rat)
	{
		return;
	}
	// Direção plausível: perto da rotação de controle que o servidor conhece.
	const FVector ControlDir = Rat->GetBaseAimRotation().Vector();
	const float CosLimit = FMath::Cos(FMath::DegreesToRadians(MaxAimDeviationDegrees));
	FVector Dir = FVector(AimDirection).GetSafeNormal();
	if (Dir.IsNearlyZero() || FVector::DotProduct(Dir, ControlDir) < CosLimit)
	{
		Dir = ControlDir; // rejeita a direção suspeita e usa a do servidor
	}
	FireFromServer(Dir);
}

bool URatWeaponComponent::FireFromServer(const FVector& AimDirection)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (!CanFireNow(Now))
	{
		return false;
	}
	LastServerFireTime = Now;
	PendingBurstDirection = AimDirection.GetSafeNormal();
	PendingBurstShots = GetStats().ShotsPerTrigger;
	FireBurstShot();
	return true;
}

void URatWeaponComponent::FireBurstShot()
{
	ARatCharacter* Rat = GetRat();
	if (!Rat || Rat->IsDead() || Ammo <= 0 || PendingBurstShots <= 0)
	{
		PendingBurstShots = 0;
		if (Ammo <= 0 && Rat && !Rat->IsDead())
		{
			StartReload();
		}
		return;
	}

	// Rajadas acompanham a mira atual (para jogadores, a rotação de controle do servidor).
	FVector Dir = PendingBurstDirection;
	if (PendingBurstShots < GetStats().ShotsPerTrigger && !Rat->IsBotControlled())
	{
		Dir = Rat->GetBaseAimRotation().Vector();
	}
	SpawnProjectile(Dir);
	--Ammo;
	--PendingBurstShots;

	if (PendingBurstShots > 0 && Ammo > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(BurstTimer, this, &URatWeaponComponent::FireBurstShot, FMath::Max(0.01f, GetStats().BurstShotInterval), false);
	}
	else
	{
		PendingBurstShots = 0;
		if (Ammo <= 0)
		{
			StartReload();
		}
	}
}

void URatWeaponComponent::SpawnProjectile(const FVector& AimDirection)
{
	ARatCharacter* Rat = GetRat();
	if (!Rat || !ProjectileClass)
	{
		return;
	}
	const FRatWeaponStats& S = GetStats();
	const FVector Dir = FMath::VRandCone(AimDirection, FMath::DegreesToRadians(S.SpreadDegrees));
	const FVector Muzzle = GetMuzzleLocation();

	ARatProjectile* P = GetWorld()->SpawnActorDeferred<ARatProjectile>(ProjectileClass, FTransform(Dir.Rotation(), Muzzle), Rat, Rat, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (P)
	{
		P->InitProjectile(S, WeaponType, Dir, Rat->GetController());
		P->FinishSpawning(FTransform(Dir.Rotation(), Muzzle));
	}
}

void URatWeaponComponent::StartReload()
{
	if (bReloading)
	{
		return;
	}
	bReloading = true;
	const AGameStateBase* GS = GetWorld()->GetGameState();
	ReloadStartServerTime = GS ? static_cast<float>(GS->GetServerWorldTimeSeconds()) : 0.f;
	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &URatWeaponComponent::FinishReload, FMath::Max(0.05f, GetStats().ReloadTime), false);
}

void URatWeaponComponent::FinishReload()
{
	bReloading = false;
	Ammo = GetStats().MagazineSize;
}
