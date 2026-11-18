#include "UI/RatSfx.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"

namespace
{
	struct FTone
	{
		float StartHz;
		float EndHz;
		float Duration;
		float Volume;
		float Noise; // 0..1 mistura de ruído (impactos)
	};

	FTone GetTone(ERatSfx Cue)
	{
		switch (Cue)
		{
		case ERatSfx::FireSlingshot: return { 420.f, 160.f, 0.12f, 0.45f, 0.25f };
		case ERatSfx::FireCaps:      return { 900.f, 600.f, 0.05f, 0.30f, 0.35f };
		case ERatSfx::HitWorld:      return { 260.f, 120.f, 0.06f, 0.30f, 0.7f };
		case ERatSfx::HitRat:        return { 520.f, 300.f, 0.08f, 0.45f, 0.4f };
		case ERatSfx::HitConfirm:    return { 1400.f, 1400.f, 0.05f, 0.35f, 0.f };
		case ERatSfx::KillConfirm:   return { 700.f, 1400.f, 0.22f, 0.45f, 0.f };
		case ERatSfx::Death:         return { 500.f, 90.f, 0.45f, 0.5f, 0.15f };
		case ERatSfx::CheeseMine:    return { 600.f, 1200.f, 0.25f, 0.5f, 0.f };
		case ERatSfx::CheeseOther:   return { 500.f, 350.f, 0.2f, 0.4f, 0.f };
		case ERatSfx::Respawn:       return { 300.f, 900.f, 0.3f, 0.35f, 0.f };
		case ERatSfx::Countdown:     return { 880.f, 880.f, 0.12f, 0.4f, 0.f };
		case ERatSfx::RoundStart:    return { 660.f, 1320.f, 0.35f, 0.5f, 0.f };
		case ERatSfx::RoundEnd:      return { 1000.f, 250.f, 0.8f, 0.5f, 0.f };
		case ERatSfx::UiClick:       return { 1200.f, 900.f, 0.04f, 0.25f, 0.f };
		default:                     return { 440.f, 440.f, 0.1f, 0.3f, 0.f };
		}
	}
}

void RatSfx::PlayCue(const UObject* WorldContext, ERatSfx Cue, const FVector* Location)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World && GEngine && GEngine->GameViewport)
	{
		World = GEngine->GameViewport->GetWorld(); // cliques de UI sem contexto
	}
	if (!World || World->GetNetMode() == NM_DedicatedServer || !GEngine || !GEngine->UseSound())
	{
		return;
	}

	const FTone T = GetTone(Cue);
	constexpr int32 SampleRate = 22050;
	const int32 NumSamples = FMath::Max(1, FMath::RoundToInt(T.Duration * SampleRate));

	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(GetTransientPackage());
	Wave->SetSampleRate(SampleRate);
	Wave->NumChannels = 1;
	Wave->Duration = T.Duration;
	Wave->SoundGroup = SOUNDGROUP_Default;
	Wave->bLooping = false;

	TArray<uint8> Bytes;
	Bytes.SetNumZeroed(NumSamples * sizeof(int16));
	int16* Samples = reinterpret_cast<int16*>(Bytes.GetData());
	float Phase = 0.f;
	FRandomStream Rng(static_cast<int32>(Cue) * 7919 + 13);
	for (int32 i = 0; i < NumSamples; ++i)
	{
		const float A = static_cast<float>(i) / NumSamples;
		const float Hz = FMath::Lerp(T.StartHz, T.EndHz, A);
		Phase += 2.f * PI * Hz / SampleRate;
		const float Env = FMath::Min(1.f, i / 60.f) * FMath::Square(1.f - A);
		const float Tone = FMath::Sin(Phase) > 0.f ? 0.6f : -0.6f; // onda quadrada suave (estilo "cartoon")
		const float Sine = FMath::Sin(Phase);
		const float Noise = Rng.FRandRange(-1.f, 1.f);
		const float S = FMath::Lerp(0.5f * Tone + 0.5f * Sine, Noise, T.Noise) * Env;
		Samples[i] = static_cast<int16>(FMath::Clamp(S, -1.f, 1.f) * 14000.f);
	}
	Wave->QueueAudio(Bytes.GetData(), Bytes.Num());

	if (Location)
	{
		UGameplayStatics::PlaySoundAtLocation(World, Wave, *Location, T.Volume);
	}
	else
	{
		UGameplayStatics::PlaySound2D(World, Wave, T.Volume);
	}
}
